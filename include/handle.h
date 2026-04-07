#ifndef HANDLE_H
#define HANDLE_H

#include <ucontext.h>

class Handle {
public:
    // Handle migration operations
    static void migration_handle(ucontext_t *uc);
    
    // Handle translation operations
    static void translation_handle(ucontext_t *uc, uintptr_t fault_pc);
};

#endif // HANDLE_H
