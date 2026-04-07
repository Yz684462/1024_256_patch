#ifndef ASM_H
#define ASM_H

#include <ucontext.h>

// Assembly macros for register context management
#define LOAD(reg, idx, uc) \
    do { \
    __asm__ volatile ( \
        "mv " #reg ", %0\n\t" \
        : \
        : "r" ((uc)->uc_mcontext.__gregs[idx]) \
        : #reg  \
    ); \
} while(0)

#define STORE(reg, idx, uc) \
    do { \
        __asm__ volatile ( \
            "mv %0, " #reg "\n\t" \
            : "=r" ((uc)->uc_mcontext.__gregs[idx]) \
            : \
            : "memory" \
        ); \
} while(0)

#endif // ASM_H
