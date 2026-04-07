#include "../include/patch.h"
#include "../include/addr.h"
#include "../include/handle.h"
#include "../include/config.h"
#include "../include/globals.h"
#include <dlfcn.h>
#include <signal.h>

// Constructor function that runs automatically when program loads
__attribute__((constructor))
void init() {
    // Set up signal handler for SIGSEGV
    struct sigaction sa;
    sa.sa_sigaction = Patch::ebreak_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGSEGV, &sa, NULL) != 0) {
        return;
    }

    // Load migration library
    global_migration_lib_handle = dlopen(config_migration_lib_name, RTLD_LAZY);
    if (!global_migration_lib_handle) {
        return;
    }
    
    // Get migration library base address
    Dl_info dl_info;
    if (dladdr((void*)global_migration_lib_handle, &dl_info) && dl_info.dli_fbase) {
        global_migration_lib_base_addr = (uint64_t)dl_info.dli_fbase;
    } else {
        return;
    }
    
    // Get migration address
    global_migration_addr = Addr::get_migration_addr(global_migration_lib_handle, config_migration_func_name, config_migration_func_offset);
    if (global_migration_addr == 0) {
        return;
    }
    
    // Patch migration address
    Patch::patch(global_migration_addr);
    
    // TODO: Implement initialization logic
    // This function will be called automatically when the program starts
    
    // Example usage of the framework components:
    // auto ranges = Addr::get_translation_ranges(global_migration_addr);
    // Handle::migration_handle();
    // Handle::translation_handle();
}
