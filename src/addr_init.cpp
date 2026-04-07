#include "addr_init.h"
#include "../include/globals.h"
#include "../include/config.h"
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <r_core.h>
#include <r_anal.h>

namespace AddrCore {

RCore* init_r_core() {
    // Initialize r_core
    RCore *core = r_core_new();
    if (!core) {
        fprintf(stderr, "Failed to create RCore\n");
        return nullptr;
    }
    
    // Open and load shared library
    if (!r_core_file_open(core, config_migration_lib_name, R_PERM_R, 0)) {
        fprintf(stderr, "Failed to open migration library: %s\n", config_migration_lib_name);
        r_core_free(core);
        return nullptr;
    }
    r_core_bin_load(core, nullptr, UT64_MAX);
    
    // Set architecture to RISC-V
    r_config_set(core->config, "asm.arch", "riscv");
    r_config_set_i(core->config, "asm.bits", 64);
    
    // Execute deep analysis
    r_core_cmd0(core, "aaa");
    
    return core;
}

RAnalFunction* find_func(uint64_t addr, RCore *core) {
    // Find function containing addr
    RList *functions = r_anal_get_fcns(core->anal);
    RAnalFunction *target_func = nullptr;
    RListIter *iter;
    void *ptr;
    
    r_list_foreach(functions, iter, ptr) {
        RAnalFunction *fcn = reinterpret_cast<RAnalFunction*>(ptr);
        ut64 fcn_size = r_anal_function_linear_size(fcn);
        if (addr >= fcn->addr && addr < fcn->addr + fcn_size) {
            target_func = fcn;
            break;
        }
    }
    
    return target_func;
}

} // namespace AddrCore
