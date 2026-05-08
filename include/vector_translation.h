#ifndef VECTOR_TRANSLATION_H
#define VECTOR_TRANSLATION_H

#include "utils.h"

#include <vector>
#include <cstdint>
#include <utility>
#include <string>
#include <linux/ptrace.h>
#include <ucontext.h>
#include <mutex>
#include <map>
#include <set>

namespace BinaryTranslation {
    
    namespace TranslationId {

        // Translation ID manager singleton
        class TranslationIdManager {
            public:
                static TranslationIdManager& getInstance();
                int get_current_translation_id();
                void set_next_pc_for_translation_id(int translation_id, uint64_t next_pc);
                uint64_t get_next_pc_for_translation_id(int translation_id);

            private:
                TranslationIdManager() = default;
                ~TranslationIdManager() = default;
                
                // Delete copy constructor and assignment operator
                TranslationIdManager(const TranslationIdManager&) = delete;
                TranslationIdManager& operator=(const TranslationIdManager&) = delete;
                
                std::map<pid_t, int> pid_tid_map_;
                std::map<int, uint64_t> id_next_pc_map_;
                std::mutex map_mutex_;
                static int translation_id_counter_;
        };

    } // namespace TranslationId

    namespace TranslationSharedLib {

        class TranslationHandleManager {
            public:
                static TranslationHandleManager& getInstance();
                void update_translation_handle(int translation_id, Instruction* inst, int vtype);
                uint64_t get_function_address(int translation_id, uint64_t addr_rela);
            
            private:
                std::map<int, std::set<uint64_t>> id_translated_addrs_map_;
                std::map<int, void*> id_handle_map_;

                std::string make_func_name(uint64_t fault_addr);
                std::string make_shared_lib_name(int translation_id);
                std::string make_assembly_name(int translation_id);
                void make_assembly(int translation_id, Instruction* inst, int vtype);
                void *recompile_handle(int translation_id);

                TranslationHandleManager() = default;
                ~TranslationHandleManager() = default;
                
                // Delete copy constructor and assignment operator
                TranslationHandleManager(const TranslationHandleManager&) = delete;
                TranslationHandleManager& operator=(const TranslationHandleManager&) = delete;
        };

    } // namespace TranslationSharedLib

    namespace VectorContext {

        // Vector context manager singleton
        class VectorContextManager {

            public:
                static VectorContextManager& getInstance();
                void copy_uc_to_vc(ucontext_t *uc, int translation_id/*, uint32_t uc_mask*/);
                void copy_vc_to_uc(int translation_id, ucontext_t *uc/*, uint32_t vc_mask*/);
                int read_vtype_from_vc(int translation_id);
            private:
                VectorContextManager();
                
                // Delete copy constructor and assignment operator
                VectorContextManager(const VectorContextManager&) = delete;
                VectorContextManager& operator=(const VectorContextManager&) = delete;

                
                uint8_t * vc_pool_;
        };
            
            
    } // namespace VectorContext
        
} // namespace BinaryTranslation
    
#endif // VECTOR_TRANSLATION_H
