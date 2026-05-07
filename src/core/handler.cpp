#include "core.h"
#include "utils.h"
#include "vector_translation.h"
#include <iostream>
#include <set>
#include <thread>

namespace BinaryTranslation {
    namespace Handler {

        void setup_signal_handler() {
            // Set up signal handler for SIGTRAP
            int signal_to_catch = SIGTRAP;
            
            struct sigaction sa;
            sa.sa_sigaction = Handler::ebreak_handler;
            sa.sa_flags = SA_SIGINFO;
            sigemptyset(&sa.sa_mask);
            if (sigaction(signal_to_catch, &sa, NULL) != 0) {
                return;
            }
        }

        void ebreak_handler(int sig, siginfo_t *info, void *context) {
            // 保存已执行迁移的线程id
            static std::set<pid_t> migrated_threads;
            
            ucontext_t *uc = (ucontext_t *)context;
            uint64_t fault_pc = (uint64_t)info->si_addr;
            
            Dump::OnlineDumpAnalyzer &dump_analyzer = Dump::OnlineDumpAnalyzer::getInstance();
            Patch::Patcher &patcher = Patch::Patcher::getInstance();
            
            Instruction *fault_instruction = dump_analyzer.addr_to_inst(fault_pc);
            
            std::cout <<"fault instruction is 0x" << std::hex << fault_pc->address << " " << fault_instruction->opcode << std::dec << std::endl;
            
            //NOTE：现在ebreak的方式，最开始指定的迁移点假设为向量指令，所以下面不取消其插桩
            if(migrated_threads.find(getpid()) == migrated_threads.end()){
                // TODO：还有需要执行<拷贝向量状态>
                // TODO: 
                // migration_handle的指责应该是对目标指令所在的函数生成翻译代码共享库+进行相应的插桩
                Handle::handle_translation_function(fault_pc);
                migrated_threads.insert(getpid());
            }
            
            // Check if it's in patched_addrs
            if (fault_instruction->opcode == "jal" || fault_instruction->opcode == "jalr") {
                std::cout << "fault function jump instruction, address = 0x" << std::hex << fault_pc->address << std::dec << std::endl;
                // TODO：先找到目标地址
                uint64_t target_addr = Handle::get_function_jump_target(uc, fault_instruction, fault_pc);
                // TODO：
                Instruction *target_instruction = dump_analyzer.addr_to_inst(target_addr);
                if (target_instruction == nullptr) {
                    std::cerr << "Error: target instruction not found for function jump at 0x" << std::hex << fault_pc->address << std::dec << std::endl;
                    _exit(1);
                }
                Handle::handle_translation_function(fault_pc);

                patcher.restore_addr(fault_pc);
                uc->uc_mcontext.__gregs[REG_PC] = fault_pc;
                // Handle::function_jump_handle(uc, fault_instruction);
                // patcher.restore_addr(fault_pc);
                // uc->uc_mcontext.__gregs[REG_PC] = fault_pc;
            }  // Error for other cases
            else if (fault_instruction->opcode[0] == 'v') {
                std::cout << "fault vector instruction, address = 0x" << std::hex << fault_pc->address << std::dec << std::endl;
                // NOTE:现在验证jal/jalr的插桩，所以直接取消向量指令的插桩，返回原向量指令执行
                patcher.restore_addr(fault_pc);
                uc->uc_mcontext.__gregs[REG_PC] = fault_pc;

                // Handle::translation_handle(uc, fault_instruction);
                // uint64_t range_end = patcher.query_range_end(fault_pc);
                // if (range_end == 0) {
                //     // TODO: handle error
                //     _exit(1);
                // }
                // uc->uc_mcontext.__gregs[REG_PC] = range_end;
            }
            else{
                throw std::runtime_error("unsupported opcode in ebreak handler: " + fault_instruction->opcode);
            }
        }

    } // namespace Handler
} // namespace BinaryTranslation
