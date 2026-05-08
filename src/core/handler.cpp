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
            sa.sa_sigaction = ebreak_handler;
            sa.sa_flags = SA_SIGINFO;
            sigemptyset(&sa.sa_mask);
            if (sigaction(signal_to_catch, &sa, NULL) != 0) {
                return;
            }
        }

        void ebreak_handler(int /*sig*/, siginfo_t *info, void *context) {
            // 保存已执行迁移的线程id
            static std::set<pid_t> migrated_threads;
            
            ucontext_t *uc = (ucontext_t *)context;
            uint64_t fault_pc = (uint64_t)info->si_addr;
            
            Dump::OnlineDumpAnalyzer &dump_analyzer = BinaryTranslation::Dump::OnlineDumpAnalyzer::getInstance();
            Patch::Patcher &patcher = BinaryTranslation::Patch::Patcher::getInstance();
            auto &transaction_id_manager = BinaryTranslation::TranslationId::TranslationIdManager::getInstance();

            Instruction *fault_instruction = dump_analyzer.addr_to_inst(fault_pc);
            int translation_id = transaction_id_manager.get_current_translation_id();

            if(fault_instruction == nullptr){
                uint64_t next_pc = transaction_id_manager.get_next_pc_for_translation_id(translation_id);
                if(next_pc != 0){
                    uc->uc_mcontext.__gregs[REG_PC] = next_pc;
                }
            }
            else{
                std::cout <<"fault instruction is 0x" << std::hex << fault_instruction->address << " " << fault_instruction->opcode << std::dec << std::endl;

                //NOTE：现在ebreak的方式，最开始指定的迁移点假设为向量指令，所以下面不取消其插桩
                if(migrated_threads.find(getpid()) == migrated_threads.end()){
                    auto &vector_context_manager = BinaryTranslation::VectorTranslation::VectorContextManager::getInstance();
                    
                    vector_context_manager.copy_uc_to_vc(uc, translation_id);
                    handle_patch_function(fault_pc);
                    migrated_threads.insert(getpid());
                }
                if (fault_instruction->opcode == "jal" || fault_instruction->opcode == "jalr") {
                    std::cout << "fault function jump instruction, address = 0x" << std::hex << fault_instruction->address << std::dec << std::endl;
    
                    uint64_t target_addr = BinaryTranslation::Helper::get_function_jump_target(uc, fault_instruction);
    
                    handle_patch_function(target_addr);
                    patcher.restore_addr(fault_pc);
                    uc->uc_mcontext.__gregs[REG_PC] = fault_pc;
                }
                else if (fault_instruction->opcode[0] == 'v') {
                    std::cout << "fault vector instruction, address = 0x" << std::hex << fault_instruction->address << std::dec << std::endl;
                    
                    auto &translation_handle_manager = BinaryTranslation::TranslationSharedLib::TranslationHandleManager::getInstance();
                    
                    int vtype = vector_context_manager.read_vtype_from_vc(translation_id);
    
                    translation_handle_manager.update_translation_handle(translation_id, fault_instruction, vtype);
                    uint64_t translated_func_addr = translation_handle_manager.get_function_address(translation_id, fault_instruction->address);
                    transaction_id_manager.set_next_pc_for_translation_id(translation_id, fault_pc + fault_instruction->instrlen);
                    uc->uc_mcontext.__gregs[REG_PC] = translated_func_addr;
                }
                else{
                    throw std::runtime_error("unsupported opcode in ebreak handler: " + fault_instruction->opcode);
                }
            }
            
        }

        void handle_patch_function(uint64_t addr_abs) {
            auto& dump_analyzer = BinaryTranslation::Dump::OnlineDumpAnalyzer::getInstance();
            auto& patcher = BinaryTranslation::Patch::Patcher::getInstance();

            const auto &insts = dump_analyzer.select_func_content(addr_abs);
            const auto &insts_to_patch = patcher.analyze_insts_to_patch(insts);
            std::vector<uint64_t> addrs_to_patch = dump_analyzer.insts_to_abs_addrs(insts_to_patch);
            for(size_t i = 0; i < addrs_to_patch.size(); i++){
                patcher.patch_addr(addrs_to_patch[i], insts_to_patch[i]);
            }
        }

    } // namespace Handler
} // namespace BinaryTranslation
