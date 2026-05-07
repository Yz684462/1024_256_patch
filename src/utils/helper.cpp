#include "utils.h"
#include <unordered_map>
#include <string>
#include <link.h>
#include <dlfcn.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <cstring>

namespace BinaryTranslation {
    namespace Helper {
        int reg_name_to_num(const std::string& reg_name) {
            static const std::unordered_map<std::string, int> reg_map = {
                {"ra", 1}, {"sp", 2}, {"gp", 3}, {"tp", 4},
                {"t0", 5}, {"t1", 6}, {"t2", 7}, {"s0", 8}, {"s1", 9},
                {"a0", 10}, {"a1", 11}, {"a2", 12}, {"a3", 13}, {"a4", 14},
                {"a5", 15}, {"a6", 16}, {"a7", 17}, {"s2", 18}, {"s3", 19},
                {"s4", 20}, {"s5", 21}, {"s6", 22}, {"s7", 23}, {"s8", 24},
                {"s9", 25}, {"s10", 26}, {"s11", 27}, {"t3", 28}, {"t4", 29},
                {"t5", 30}, {"t6", 31}
            };
            
            auto it = reg_map.find(reg_name);
            if (it != reg_map.end()) {
                return it->second;
            }
            return -1;
        }

        template<typename Func>
        static int callback_wrapper(struct dl_phdr_info *info, size_t size, void *data) {
            return (*static_cast<Func*>(data))(info, size);
        }

        uint64_t get_shared_lib_base_addr(const std::string& shared_lib_name) {
            uint64_t base_addr = 0;
            
            auto callback = [&](struct dl_phdr_info *info, size_t /*size*/) -> int {
                if (info->dlpi_name && strstr(info->dlpi_name, shared_lib_name.c_str())) {
                    base_addr = info->dlpi_addr;
                    return 1;
                }
                return 0;
            };
            
            // 使用包装函数传递 lambda
            dl_iterate_phdr(callback_wrapper<decltype(callback)>, &callback);
            
            return base_addr;
        }

        void write_content_to_file(const std::string& filename, const std::string& content) {
            std::ofstream file(filename);
            if (!file.is_open()) {
                std::cerr << "[ERROR] Failed to open file: " << filename << std::endl;
                throw std::runtime_error("Failed to open file for writing");
            }
            
            file << content;
            file.close();
        }
    }
}