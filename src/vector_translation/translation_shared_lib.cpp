#include "vector_translation.h"
#include "types.h"

#include <dlfcn.h>
#include <iostream>

namespace BinaryTranslation {
    namespace TranslationSharedLib {

        TranslationHandleManager& TranslationHandleManager::getInstance() {
            static TranslationHandleManager instance;
            return instance;
        }

        std::string TranslationHandleManager::make_func_name(uint64_t addr_rela) {
            return "translation_func_" + std::to_string(addr_rela);
        }

        std::string TranslationHandleManager::make_shared_lib_name(int translation_id) {
            return "translation_lib_" + std::to_string(translation_id) + ".so";
        }
        
        std::string TranslationHandleManager::make_assembly_name(int translation_id) {
            return "translation_asm_" + std::to_string(translation_id) + ".s";
        }

        void TranslationHandleManager::make_assembly(int translation_id, Instruction* inst, int vtype) {
            // 准备脚本参数
            std::string content_to_translate = inst->line;

            std::string translation_func_name = make_func_name(inst->address);

            std::string translation_assembly_name = make_assembly_name(translation_id);

            // 执行脚本
            std::string command = "python3 scripts/translator.py " + 
                std::to_string(translation_id) + " " +
                "\"" + content_to_translate + "\"" + " " + 
                translation_func_name + " " +
                vtype + " >> " + 
                translation_assembly_name;
            int result_gen_assembly = system(command.c_str());
            if (result_gen_assembly != 0) {
                // Handle error if needed
            }
        }
        
        void *TranslationHandleManager::recompile_handle(int translation_id){
            // 编译共享库并更新handle map
            std::string translation_shared_lib_name = make_shared_lib_name(translation_id);
            std::string translation_assembly_name = make_assembly_name(translation_id);

            std::string command = "g++ -shared -fPIC -o " + translation_shared_lib_name + " " + translation_assembly_name;
            int result_gen_shared_lib = system(command.c_str());
            if (result_gen_shared_lib != 0) {
                // Handle error if needed
            }
            
            void *translation_handle = dlopen(translation_shared_lib_name.c_str(), RTLD_LAZY);
            if (translation_handle == nullptr) {
                std::cerr << "Failed to load translation shared library: " << dlerror() << std::endl;
                return;
            }
            return translation_handle;
        }
        
        uint64_t TranslationHandleManager::get_function_address(int translation_id, uint64_t addr_rela){
            std::string func_name = make_func_name(addr_rela);
            void *translation_handle = id_handle_map_[translation_id];
            void *func_ptr = dlsym(translation_handle, func_name.c_str());
            if (func_ptr == nullptr) {
                std::cerr << "Failed to load translation function: " << dlerror() << std::endl;
                return 0;
            }
            return (uint64_t)func_ptr;
        }

        void TranslationHandleManager::update_translation_handle(int translation_id, Instruction* inst, int vtype) {
            if(id_translated_addrs_map_[translation_id].count(inst->address) == 0) {
                make_assembly(translation_id, inst, vtype);
                id_translated_addrs_map_[translation_id].insert(inst->address);
                void *translation_handle = recompile_handle(translation_id);
                id_handle_map_[translation_id] = translation_handle;
            }
        }
    } // namespace TranslationSharedLib
} // namespace BinaryTranslation