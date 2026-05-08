#include "vector_translation.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ucontext.h>
#include <dlfcn.h>
#include <iostream>


#define VECTOR_CONTEXT_SIZE 4192

namespace BinaryTranslation {
    namespace VectorContext {

        VectorContextManager::VectorContextManager() {
            void* data_handle = dlopen("./libdata.so", RTLD_NOW | RTLD_GLOBAL);
            vc_pool_ = (uint8_t *)dlsym(data_handle, "simulated_cpu_state");
        }

        VectorContextManager& VectorContextManager::getInstance() {
            static VectorContextManager instance;
            return instance;
        }

        int VectorContextManager::read_vtype_from_vc(int translation_id) {
            uint8_t* simulated_context = vc_pool_ + translation_id * VECTOR_CONTEXT_SIZE;
            uint64_t vtype = *(uint64_t*)(simulated_context + 0x1028);
            return (int)vtype;
        }
        
        void VectorContextManager::copy_uc_to_vc(ucontext_t *uc, int translation_id /*, uint32_t uc_mask*/) {
            // Get OS vector context
            struct __riscv_v_ext_state* os_vector_context = BinaryTranslation::Helper::get_os_vector_context(uc);
            
            // Save OS vector context to simulated context
            uint8_t* simulated_context = vc_pool_ + translation_id * VECTOR_CONTEXT_SIZE;
            
            // Save vector state
            *(uint64_t*)(simulated_context + 0x1000) = (uint64_t)os_vector_context->vstart;
            //vxsat not handled
            //vxrm not handled
            *(uint64_t*)(simulated_context + 0x1018) = (uint64_t)os_vector_context->vcsr;
            *(uint64_t*)(simulated_context + 0x1020) = (uint64_t)os_vector_context->vl;
            *(uint64_t*)(simulated_context + 0x1028) = (uint64_t)os_vector_context->vtype;
            *(uint64_t*)(simulated_context + 0x1030) = (uint64_t)os_vector_context->vlenb;
            
            // Save v0 to v31
            size_t total_size = 32 * (uint64_t)os_vector_context->vlenb;
            memcpy(simulated_context, os_vector_context->datap, total_size);
        }

        void VectorContextManager::copy_vc_to_uc(int translation_id, ucontext_t *uc /*, uint32_t vc_mask*/) {
            // Get OS vector context
            struct __riscv_v_ext_state* os_vector_context = BinaryTranslation::Helper::get_os_vector_context(uc);
            
            // Restore simulated vector context to OS vector context
            uint8_t* simulated_context = (uint8_t*)vc_pool_ + translation_id * VECTOR_CONTEXT_SIZE;

            // Restore vector state from simulated context
            os_vector_context->vstart = *(uint64_t*)(simulated_context + 0x1000);
            //vxsat not handled
            //vxrm not handled
            os_vector_context->vcsr = *(uint64_t*)(simulated_context + 0x1018);
            os_vector_context->vl = *(uint64_t*)(simulated_context + 0x1020);
            os_vector_context->vtype = *(uint64_t*)(simulated_context + 0x1028);
            os_vector_context->vlenb = *(uint64_t*)(simulated_context + 0x1030);
            
            // Restore vector data using same logic as save
            int vlen = (uint64_t)os_vector_context->vlenb * 8;
            size_t total_size = 32 * vlen;
            memcpy(os_vector_context->datap, simulated_context, total_size);
        }

    } // namespace VectorContext
} // namespace BinaryTranslation
