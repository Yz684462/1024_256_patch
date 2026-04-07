#ifndef ADDR_INIT_H
#define ADDR_INIT_H

#include <r_core.h>
#include <r_anal.h>

namespace AddrCore {

// Initialize r_core with migration library
RCore* init_r_core();

// Find function containing address
RAnalFunction* find_func(uint64_t addr, RCore *core);

} // namespace AddrCore

#endif // ADDR_INIT_H
