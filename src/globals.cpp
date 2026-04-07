#include "../include/globals.h"

// Global variables definition
void* global_migration_lib_handle = nullptr;
uint64_t global_migration_lib_base_addr = 0;
uint64_t global_migration_addr = 0;
std::vector<uint64_t> global_patched_addrs;
std::vector<std::pair<uint64_t, uint64_t>> global_translation_ranges;
std::map<std::thread::id, int> global_thread_to_index_map;
void* global_simulated_vector_contexts_pool = nullptr;
int global_next_context_index = 0;
std::map<int, void*> global_thread_translated_handles;
