#ifndef PATCH_H
#define PATCH_H

#include <cstdint>
#include <signal.h>

class Patch {
public:
    // Patch function at given address
    static void patch(uint64_t addr);
    
    // Handle ebreak exception
    static void ebreak_handler(int sig, siginfo_t *info, void *context);
};

#endif // PATCH_H
