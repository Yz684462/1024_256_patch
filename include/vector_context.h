#ifndef VECTOR_CONTEXT_H
#define VECTOR_CONTEXT_H

#include <ucontext.h>
#include <vector>
#include <thread>

#define RISCV_V_MAGIC		0x53465457

struct __riscv_v_ext_state;

// Global variables for simulated vector contexts
extern std::map<std::thread::id, int> global_thread_to_index_map;
extern void* global_simulated_vector_contexts_pool;
extern int global_next_context_index;

class VectorContext {
public:
    // Get OS vector context from ucontext
    static struct __riscv_v_ext_state* get_os_vector_context(ucontext_t *uc);
    
    // Save OS vector context to simulated vector context
    static void save_os_vector_context_to_simulated_vector_context(struct __riscv_v_ext_state *os_vector_context, int thread_index);
    
    // Save simulated vector context to OS vector context
    static void save_simulated_vector_context_to_os_vector_context(struct __riscv_v_ext_state *os_vector_context, int thread_index);
    
    // Initialize the vector context pool
    static void initialize_vector_context_pool();
    
    // Get or assign thread index (only for migration_handle)
    static int get_or_assign_thread_index();
    
    // Get thread index only (for other functions)
    static int get_thread_index();
};

#endif // VECTOR_CONTEXT_H
