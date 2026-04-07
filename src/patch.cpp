#include "patch.h"
#include "../include/globals.h"
#include "../include/handle.h"
#include <sys/mman.h>
#include <unistd.h>
#include <ucontext.h>
#include <algorithm>

void Patch::patch(uint64_t addr) {
    size_t page_size = getpagesize();
    uintptr_t page_start = addr & ~(page_size - 1);

    // 修改访问权限使得可修改
    if (mprotect((void*)page_start, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        return;
    }

    uint32_t* ins_ptr = (uint32_t*)addr;
    
    // 修改成ebreak
    ins_ptr[0] = 0x00100073; // EBREAK
    
    __builtin___clear_cache((char*)addr, (char*)addr + 4);
    
    // 将修改的地址注册到global_patched_addrs
    global_patched_addrs.push_back(addr);
}

void Patch::ebreak_handler(int sig, siginfo_t *info, void *context) {
    ucontext_t *uc = (ucontext_t *)context;
    uintptr_t fault_pc = (uintptr_t)info->si_addr;
    
    // 检查是否是migration_addr
    if (fault_pc == global_migration_addr) {
        Handle::migration_handle(uc);
        uc->uc_mcontext.__gregs[REG_PC] = fault_pc;
    }
    // 检查是否在patched_addrs中
    } else if (std::find(global_patched_addrs.begin(), global_patched_addrs.end(), fault_pc) != global_patched_addrs.end()) {
            Handle::translation_handle(uc, fault_pc);
            uint64_t range_end = Addr::get_translation_range_end(fault_pc);
            uc->uc_mcontext.__gregs[REG_PC] = range_end;
        }  // 其他情况报错
    else {
        _exit(1);
    }
}
