#include "utils.h"
#include "types.h"

#include <unordered_map>
#include <string>
#include <link.h>
#include <dlfcn.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <cstring>

namespace BinaryTranslation {
    namespace Helper {
        int reg_name_to_num(const std::string& reg_name) {
            static const std::unordered_map<std::string, int> reg_map = {
                {"ra", 1}, {"sp", 2}, {"gp", 3}, {"tp", 4},
                {"t0", 5}, {"t1", 6}, {"t2", 7}, {"s0", 8}, {"s1", 9},
                {"a0", 10}, {"a1", 11}, {"a2", 12}, {"a3", 13}, {"a4", 14},
                {"a5", 15}, {"a6", 16}, {"a7", 17}, {"s2", 18}, {"s3", 19},
                {"s4", 20}, {"s5", 21}, {"s6", 22}, {"s7", 23}, {"s8", 24},
                {"s9", 25}, {"s10", 26}, {"s11", 27}, {"t3", 28}, {"t4", 29},
                {"t5", 30}, {"t6", 31}
            };
            
            auto it = reg_map.find(reg_name);
            if (it != reg_map.end()) {
                return it->second;
            }
            return -1;
        }

        template<typename Func>
        static int callback_wrapper(struct dl_phdr_info *info, size_t size, void *data) {
            return (*static_cast<Func*>(data))(info, size);
        }

        uint64_t get_shared_lib_base_addr(const std::string& shared_lib_name) {
            uint64_t base_addr = 0;
            
            auto callback = [&](struct dl_phdr_info *info, size_t /*size*/) -> int {
                if (info->dlpi_name && strstr(info->dlpi_name, shared_lib_name.c_str())) {
                    base_addr = info->dlpi_addr;
                    return 1;
                }
                return 0;
            };
            
            // 使用包装函数传递 lambda
            dl_iterate_phdr(callback_wrapper<decltype(callback)>, &callback);
            
            return base_addr;
        }

        #define RISCV_V_MAGIC	0x53465457
        struct __riscv_v_ext_state* get_os_vector_context(ucontext_t *uc) {
            struct __riscv_extra_ext_header *ext;
            struct __riscv_v_ext_state *v_ext_state;
            
            ext = (struct __riscv_extra_ext_header *)(&uc->uc_mcontext.__fpregs);
            if (ext->hdr.magic != RISCV_V_MAGIC) {
                fprintf(stderr, "bad vector magic: %x\n", ext->hdr.magic);
                abort();
            }
            
            v_ext_state = (struct __riscv_v_ext_state *)((char *)(ext) + sizeof(*ext));
            return v_ext_state;
        }

        uint64_t get_function_jump_target(ucontext_t *uc, Instruction *fault_instruction) {
            uint64_t target_addr = 0;
            if (fault_instruction->opcode == "jal"){
                // 立即数是有符号数
                auto &dump_analyzer = BinaryTranslation::Dump::OnlineDumpAnalyzer::getInstance();
                target_addr = dump_analyzer.to_abs(std::stoull(fault_instruction->operands[0], nullptr, 16));
            }
            else if(fault_instruction->opcode == "jalr"){
                const std::string &target_reg = fault_instruction->operands[1];
                int target_reg_index = reg_name_to_num(target_reg);
                if (target_reg_index == -1) {
                    printf("Error: invalid register name: %s\n", target_reg.c_str());
                    return 0;
                }
                target_addr = uc->uc_mcontext.__gregs[target_reg_index];
            }
            else {
                printf("Error: unsupported opcode: %s\n", fault_instruction->opcode.c_str());
                return 0;
            }
            if (target_addr == 0) {
                printf("Error: invalid target address: 0\n");
                return 0;
            }
            return target_addr;
        }
    }
}