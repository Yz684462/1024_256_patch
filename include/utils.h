#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include <memory>
#include <ucontext.h>
#include <linux/ptrace.h>
#include <cstdint>

#include "types.h"

namespace BinaryTranslation {

    namespace Helper {
        int reg_name_to_num(std::string reg_name);
        uint64_t get_shared_lib_base_addr(const std::string& shared_lib_name);
        struct __riscv_v_ext_state* get_os_vector_context(ucontext_t *uc);
        uint64_t get_function_jump_target(ucontext_t *uc, Instruction *fault_instruction);
    } // namespace Helper

    namespace Dump {
        enum class SaveFormat {
            Binary,  // 二进制格式（最小、最快）
            JSON,    // JSON 格式（可读性好，便于调试）
            XML      // XML 格式（可读性好，兼容性好）
        };

        class BaseDumpAnalyzer{
            public:
                // 地址 -> 指令智能指针
                std::map<uint64_t, std::shared_ptr<Instruction>> addr2inst;
                
                // 地址 -> 函数名
                std::map<uint64_t, std::string> addr2func_name;
                
                // 函数名 -> 指令智能指针列表
                std::map<std::string, std::vector<std::shared_ptr<Instruction>>> func_name2insts;

                void clear_data();
        };

        class OfflineDumpAnalyzer: public BaseDumpAnalyzer{
            public:
                void scan_dump_file(const std::string& filename);
                void save_to_file(const std::string& filename, SaveFormat format);
        };
        
        class OnlineDumpAnalyzer: public BaseDumpAnalyzer{
            private:
                uint64_t base_addr;

                
                OnlineDumpAnalyzer(const uint64_t base_addr);
                ~OnlineDumpAnalyzer() = default;
                OnlineDumpAnalyzer(const OnlineDumpAnalyzer&) = delete;
                OnlineDumpAnalyzer& operator=(const OnlineDumpAnalyzer&) = delete;
                
            public:
                uint64_t to_abs(uint64_t rela_addr);
                uint64_t to_rela(uint64_t abs_addr);
                static OnlineDumpAnalyzer& getInstance(const uint64_t base_addr = 0);
                void load_from_file(const std::string& filename, SaveFormat format);
                std::vector<Instruction*> select_func_content(uint64_t addr_inside_abs);
                Instruction* addr_to_inst(uint64_t addr_abs);
                std::vector<uint64_t> insts_to_abs_addrs(const std::vector<Instruction*>& insts);
        };
    } // namespace Dump


    namespace ControlFlow {
        // Control flow utilities

        inline const std::vector<std::string> jmp_instr = {"j", "jal"};
        inline const std::vector<std::string> branch_instr = {"beq", "bne", "beqz", "bnez", "blt", "bge", "bltz", "bgez", "bltu", "bgeu", "blez", "bgtz"};
        inline const std::vector<std::string> return_instr = {"ret"};
        inline const std::vector<std::string> jmp_indirect_instr = {"jalr", "jr"};
        inline const std::vector<std::string> other_instr = {"ebreak"};

        std::vector<CodeBlock*> get_codeblocks_linear(const std::vector<Instruction*>& instructions);

    } // namespace ControlFlow

} // namespace BinaryTranslation

#endif // UTILS_H