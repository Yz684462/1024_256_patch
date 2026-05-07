#ifndef CORE_H
#define CORE_H

#include <ucontext.h>
#include <signal.h>
#include <utility>
#include <map>
#include <cstdint>

#include "types.h"

namespace BinaryTranslation {
    namespace Migration {
        extern uint64_t migration_addr;
    }

    namespace Handle {

        // Handle functions
        // void migration_handle(ucontext_t *uc, Instruction *fault_instruction);
        // void translation_handle(ucontext_t *uc, Instruction *fault_instruction);
        // void function_jump_handle(ucontext_t *uc, Instruction *fault_instruction);
        // void handle_scalar_register_write(ucontext_t *uc, Instruction *fault_instruction);
        
        void handle_translation_function(uint64_t addr_abs);
        uint64_t get_function_jump_target(ucontext_t *uc, Instruction *fault_instruction);

    } // namespace Handle

    namespace Handler {

        // Signal handling
        void setup_signal_handler();
        void ebreak_handler(int sig, siginfo_t *info, void *context);

    } // namespace Handler

    namespace Patch {
        class PageProtector {
            private:
                uintptr_t page_start;
                size_t page_size;
                int original_prot;
                bool is_protected;
                
            public:
                PageProtector(uintptr_t addr_abs);
                ~PageProtector();
                
                // 禁止拷贝
                PageProtector(const PageProtector&) = delete;
                PageProtector& operator=(const PageProtector&) = delete;
        };

        struct PatchData {
            union {
                uint16_t original_bytes_16;
                uint32_t original_bytes_32;
            };
            int inst_len;
        };

        class Patcher{
            public:
                static Patcher& getInstance();
                // Patch functions
                void patch_addr(uint64_t addr_abs, Instruction* instr);
                void restore_addr(uint64_t addr_abs);

            private:
                Patcher() = default;
                ~Patcher() = default;
                
                // Delete copy constructor and assignment operator
                Patcher(const Patcher&) = delete;
                Patcher& operator=(const Patcher&) = delete;

                std::map<uint64_t, PatchData> addr_patch_data_;
        };

    } // namespace Patch

} // namespace BinaryTranslation

#endif // CORE_H
