#include "vector_translation.h"
#include <unistd.h>

namespace BinaryTranslation {
    namespace TranslationId {

        // Initialize static counter
        int TranslationIdManager::translation_id_counter_ = 0;

        TranslationIdManager& TranslationIdManager::getInstance() {
            static TranslationIdManager instance;
            return instance;
        }

        int TranslationIdManager::get_current_translation_id() {
            pid_t current_pid = getpid();
            
            // First try to find without lock
            auto it = pid_tid_map_.find(current_pid);
            if (it != pid_tid_map_.end()) {
                return it->second;
            }
            
            // If not found, acquire lock and try again
            std::lock_guard<std::mutex> lock(map_mutex_);
            
            // If still not found, assign new translation ID
            int new_id = translation_id_counter_++;
            pid_tid_map_[current_pid] = new_id;
            
            return new_id;
        }

        void TranslationIdManager::set_next_pc_for_translation_id(int translation_id, uint64_t next_pc) {
            id_next_pc_map_[translation_id] = next_pc;
        }

        uint64_t TranslationIdManager::get_next_pc_for_translation_id(int translation_id) {
            auto it = id_next_pc_map_.find(translation_id);
            if (it != id_next_pc_map_.end()) {
                uint64_t next_pc = it->second;
                id_next_pc_map_.erase(it); // Optionally erase after retrieving
                return next_pc;
            }
            return 0; // Return 0 if not found, or consider throwing an exception
        }

    } // namespace TranslationId
} // namespace BinaryTranslation