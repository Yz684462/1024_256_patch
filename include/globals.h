#ifndef GLOBALS_H
#define GLOBALS_H

#include <cstdint>
#include <vector>
#include <map>
#include <thread>
#include "config.h"

// Global variables
extern void* global_migration_lib_handle;
extern uint64_t global_migration_lib_base_addr;  // Migration library base address
extern uint64_t global_migration_addr;
extern std::vector<uint64_t> global_patched_addrs;
extern std::vector<std::pair<uint64_t, uint64_t>> global_translation_ranges;
extern std::map<std::thread::id, int> global_thread_to_index_map;
extern void* global_simulated_vector_contexts_pool;  // 连续内存池
extern int global_next_context_index;                // 下一个可用的索引
extern std::map<int, void*> global_thread_translated_handles;  // 每个线程索引对应的翻译库句柄

#endif // GLOBALS_H
