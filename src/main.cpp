#include "types.h"
#include "core.h"
#include "utils.h"
#include <iostream>
#include <thread>
#include <string>
#include <fstream>

namespace BinaryTranslation {

    uint64_t Migration::migration_addr;

    void init_migration(){
        std::string dump_data_file_name = "dump.bin";
        std::string dump_file_name = "dump.s";
        std::string shared_lib_name = "libggml-cpu.so.0";

        // 初始化dump分析器
        std::ifstream dump_data_file(dump_data_file_name);
        if (!dump_data_file.good()) {
            auto& offline_dump_analyzer = Dump::OfflineDumpAnalyzer::getInstance();
            offline_dump_analyzer.scan_dump_file(dump_file_name);
            offline_dump_analyzer.save_to_file(dump_data_file_name, Dump::SaveFormat::BINARY);
        }
        uint64_t base_addr = Helper::get_shared_lib_base_addr(shared_lib_name);
        if (base_addr == 0) {
            std::cout << "Error getting shared library base address, pid = " << getpid() << std::endl;
            return;
        }
        auto& dump_analyzer = Dump::OnlineDumpAnalyzer::getInstance(base_addr);
        dump_analyzer.load_from_file(dump_data_file_name, Dump::SaveFormat::BINARY);


        // patch迁移点
        uint64_t migration_offset = 0x000;
        
        Migration::migration_addr = base_addr + migration_offset;
        Instruction* migration_instr = dump_analyzer.addr_to_inst(Migration::migration_addr);        
        auto &patcher = Patch::Patcher::getInstance();
        patcher.patch_addr(Migration::migration_addr, migration_instr);
    }

    __attribute__((constructor))
    void init() {
        std::cout << "running init " << std::endl;
        Handler::setup_signal_handler();
        std::cout << "running setup_signal_handler " << std::endl;
        init_migration();
        std::cout << "finished init_migration " << std::endl;
    }

} // namespace BinaryTranslation