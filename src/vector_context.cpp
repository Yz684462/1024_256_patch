#include "vector_context.h"
#include "../include/globals.h"
#include "../include/config.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <linux/ptrace.h>

#define RISCV_V_MAGIC		0x53465457

struct __riscv_v_ext_state* VectorContext::get_os_vector_context(ucontext_t *uc) {
    struct __riscv_extra_ext_header *ext;
    struct __riscv_v_ext_state *v_ext_state;
    
    ext = (void *)(&uc->uc_mcontext.__fpregs);
    if (ext->hdr.magic != RISCV_V_MAGIC) {
        fprintf(stderr, "bad vector magic: %x\n", ext->hdr.magic.magic);
        abort();
    }
    
    v_ext_state = (void *)((char *)(ext) + sizeof(*ext));
    return v_ext_state;
}

void VectorContext::initialize_vector_context_pool() {
    if (global_simulated_vector_contexts_pool == nullptr) {
        // Allocate continuous memory pool for all vector contexts
        size_t pool_size = MAX_THREADS * VECTOR_CONTEXT_SIZE;
        global_simulated_vector_contexts_pool = malloc(pool_size);
        if (!global_simulated_vector_contexts_pool) {
            fprintf(stderr, "Failed to allocate vector context pool\n");
            abort();
        }
        global_next_context_index = 0;
    }
}

int VectorContext::get_thread_index() {
    std::thread::id tid = std::this_thread::get_id();
    
    // Check if thread has an assigned index
    auto it = global_thread_to_index_map.find(tid);
    if (it != global_thread_to_index_map.end()) {
        return it->second;
    }
    
    // Thread not found - this should not happen if migration_handle was called first
    fprintf(stderr, "Thread index not found - migration_handle must be called first\n");
    abort();
}

int VectorContext::get_or_assign_thread_index() {
    std::thread::id tid = std::this_thread::get_id();
    
    // Check if thread already has an assigned index
    auto it = global_thread_to_index_map.find(tid);
    if (it != global_thread_to_index_map.end()) {
        return it->second;
    }
    
    // Check if we have available slots
    if (global_next_context_index >= MAX_THREADS) {
        fprintf(stderr, "Maximum thread limit reached\n");
        abort();
    }
    
    // Assign new index for this thread
    int index = global_next_context_index++;
    global_thread_to_index_map[tid] = index;
    return index;
}

void VectorContext::save_os_vector_context_to_simulated_vector_context(struct __riscv_v_ext_state *os_vector_context, int thread_index) {
    // Initialize pool if not done yet
    initialize_vector_context_pool();
    
    // Calculate the address of this thread's vector context
    void* simulated_context = (char*)global_simulated_vector_contexts_pool + thread_index * VECTOR_CONTEXT_SIZE;

        // 保存向量状态
    *(uint64_t*)(simulated_context + 0x1000) = (uint64_t)os_vector_context->vstart;
        //vxsat没有处理
        //vxrm没有处理
    *(uint64_t*)(simulated_context + 0x1018) = (uint64_t)os_vector_context->vcsr;
    *(uint64_t*)(simulated_context + 0x1020) = (uint64_t)os_vector_context->vl;
    *(uint64_t*)(simulated_context + 0x1028) = (uint64_t)os_vector_context->vtype;
    *(uint64_t*)(simulated_context + 0x1030) = (uint64_t)os_vector_context->vlenb;

    // 保存 v0 到 v31
    int vlen = (uint64_t)os_vector_context->vlenb * 8;
    size_t total_size = 32 * vlen;
    memcpy(simulated_context, os_vector_context->vdata, total_size);
}

void VectorContext::save_simulated_vector_context_to_os_vector_context(struct __riscv_v_ext_state *os_vector_context, int thread_index) {
    
    // Calculate the address of this thread's vector context
    void* simulated_context = (char*)global_simulated_vector_contexts_pool + thread_index * VECTOR_CONTEXT_SIZE;

    // Restore vector state from simulated context
    os_vector_context->vstart = *(uint64_t*)(simulated_context + 0x1000);
    //vxsat没有处理
    //vxrm没有处理
    os_vector_context->vcsr = *(uint64_t*)(simulated_context + 0x1018);
    os_vector_context->vl = *(uint64_t*)(simulated_context + 0x1020);
    os_vector_context->vtype = *(uint64_t*)(simulated_context + 0x1028);
    os_vector_context->vlenb = *(uint64_t*)(simulated_context + 0x1030);
    
    // Restore vector data using same logic as save
    int vlen = (uint64_t)os_vector_context->vlenb * 8;
    size_t total_size = 32 * vlen;
    memcpy(os_vector_context->vdata, simulated_context, total_size);
}
