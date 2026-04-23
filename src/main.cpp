#include "core.h"
#include "utils.h"
#include <dlfcn.h>
#include <iostream> 

namespace BinaryTranslation {

uint64_t Migration::migration_addr;

void init_migration(){
    // dump_file_name = "dump.s"
    Dump::DumpAnalyzer::getInstance("../dump.s");
    
    // shared_lib_path = "libggml-cpu.so"
    uint64_t base_addr = Addr::get_shared_lib_base_addr("libggml-cpu.so");
    if (base_addr == 0) {
        std::cerr << "Error getting shared library base address" << std::endl;
        return;
    }
    
    auto &addr_manager = Addr::AddrManager::getInstance(base_addr);   
    
    // migration_addr = 0x12345678
    Migration::migration_addr = addr_manager.to_abs(0x107f30);
    
    // patch migration addr
    auto &patcher = Patch::Patcher::getInstance();
    patcher.patch_addr(Migration::migration_addr);
}

__attribute__((constructor))
void init() {
    Handler::setup_signal_handler();
    init_migration();
}

} // namespace BinaryTranslation