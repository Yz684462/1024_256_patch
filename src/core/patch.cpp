#include "core.h"
#include <sys/mman.h>
#include <iostream> 


namespace BinaryTranslation {
    namespace Patch {
        PageProtector::PageProtector(uintptr_t addr_abs): page_size(getpagesize()), is_protected(false) {
            page_start = addr_abs & ~(page_size - 1);

            // 保存原始权限（简化：假设为 RX）
            original_prot = PROT_READ | PROT_EXEC;
            
            // 临时添加写权限
            int new_prot = original_prot | PROT_WRITE;
            if (mprotect((void*)page_start, page_size, new_prot) != 0) {
                std::cout << "Failed to add write permission: " << strerror(errno) << std::endl;
                throw std::runtime_error("mprotect failed");
            }
            is_protected = true;
        }

        PageProtector::~PageProtector() {
            if (is_protected) {
                // 恢复原始权限
                if (mprotect((void*)page_start, page_size, original_prot) != 0) {
                    std::cout << "Failed to restore original permission: " << strerror(errno) << std::endl;
                }
            }
        }



        Patcher& Patcher::getInstance() {
            static Patcher instance;
            return instance;
        }

        void Patcher::patch_addr(uint64_t addr_abs, Instruction* instr) {
            
            PageProtector protector(addr_abs);

            int instr_len = instr->instrlen;
            if( instr_len == 2){
                uint16_t* ptr = (uint16_t*)addr_abs;
                addr_patch_data_[addr_abs] = {.original_bytes_16 = *ptr, .inst_len = 2};
                *ptr = 0x9002; // 2 bytes ebreak
            }
            else if( instr_len == 4){
                uint32_t* ptr = (uint32_t*)addr_abs;
                addr_patch_data_[addr_abs] = {.original_bytes_32 = *ptr, .inst_len = 4};
                *ptr = 0x00100073; // 4 bytes ebreak
            }
            else {
                std::cout << "Error: unsupported instruction length: " << instr_len << std::endl;
                return;
            }

            __builtin___clear_cache((void*)addr_abs, (void*)(addr_abs + instr_len));
        }

        void Patcher::restore_addr(uint64_t addr_abs) {
            auto it = addr_patch_data_.find(addr_abs);
            if (it == addr_patch_data_.end()) {
                return;
            }

            PageProtector protector(addr_abs);
            
            PatchData& patch_data = it->second;
            if (patch_data.inst_len == 2) {
                uint16_t* ptr = (uint16_t*)addr_abs;
                *ptr = patch_data.original_bytes_16;
            } else if (patch_data.inst_len == 4) {
                uint32_t* ptr = (uint32_t*)addr_abs;
                *ptr = patch_data.original_bytes_32;
            }

            __builtin___clear_cache((void*)addr_abs, (void*)(addr_abs + patch_data.inst_len));

            addr_patch_data_.erase(it);
        }
    } // namespace Patch
} // namespace BinaryTranslation