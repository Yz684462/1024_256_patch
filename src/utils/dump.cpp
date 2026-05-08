#include "utils.h"
#include "cereal/archives/xml.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/archives/binary.hpp"
#include "cereal/types/map.hpp"
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>

#include <fstream>
#include <sstream>
#include <utility>
#include <iostream>
#include <stdexcept>

namespace BinaryTranslation {
    namespace Dump {
        void BaseDumpAnalyzer::clear_data(){
            addr2inst.clear();
            addr2func_name.clear();
            func_name2insts.clear();
        }

        void OfflineDumpAnalyzer::scan_dump_file(const std::string& filename) {
            clear_data();
            
            std::ifstream file(filename);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open file: " + filename);
            }
            
            std::string line;
            std::string current_func_name;
            
            while (std::getline(file, line)) {
                
                if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
                    continue;
                }
                
                // 函数定义行
                size_t lt_pos = line.find('<');
                size_t gt_pos = line.find('>');
                size_t colon_pos = line.find(':', gt_pos);
                
                if (lt_pos != std::string::npos && gt_pos != std::string::npos && colon_pos != std::string::npos) {
                    std::stringstream ss(line);
                    std::string addr_str;
                    ss >> addr_str;
                    
                    if (!addr_str.empty() && addr_str.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos) {
                        try {
                            size_t name_start = lt_pos + 1;
                            std::string func_name = line.substr(name_start, gt_pos - name_start);
                            
                            func_name2insts[func_name] = std::vector<std::shared_ptr<Instruction>>();
                            current_func_name = func_name;                            
                            continue;
                        } catch (...) {}
                    }
                }
                
                // 指令行
                size_t colon_pos_instr = line.find(':');
                if (colon_pos_instr != std::string::npos) {
                    std::stringstream ss(line);
                    std::string addr_str, machine_code, opcode;
                    
                    if (std::getline(ss, addr_str, ':')) {
                        size_t start = addr_str.find_first_not_of(" \t");
                        if (start != std::string::npos) {
                            addr_str = addr_str.substr(start);
                        } else {
                            continue;
                        }
                        
                        if (addr_str.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
                            continue;
                        }
                        
                        if (!(ss >> machine_code)) continue;
                        if (machine_code.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) continue;
                        if (!(ss >> opcode)) continue;
                        
                        std::string operand_str;
                        std::getline(ss, operand_str);
                        
                        size_t comment_pos = operand_str.find_first_of("#<");
                        if (comment_pos != std::string::npos) {
                            operand_str = operand_str.substr(0, comment_pos);
                        }
                        
                        start = operand_str.find_first_not_of(" \t");
                        if (start != std::string::npos) {
                            size_t end = operand_str.find_last_not_of(" \t");
                            operand_str = operand_str.substr(start, end - start + 1);
                        } else {
                            operand_str.clear();
                        }
                        
                        try {
                            uint64_t address = std::stoull(addr_str, nullptr, 16);
                            
                            if (machine_code.length() % 2 != 0) continue;
                            int instrlen = machine_code.length() / 2;
                            
                            // 使用 make_shared 创建智能指针
                            auto instr = std::make_shared<Instruction>(line, opcode, operand_str, address, instrlen);
                            
                            addr2inst[address] = instr;
                            addr2func_name[address] = current_func_name;
                            
                            if (!current_func_name.empty()) {
                                auto it = func_name2insts.find(current_func_name);
                                if (it != func_name2insts.end()) {
                                    it->second.push_back(instr);
                                }
                            }
                            
                        } catch (const std::exception& e) {
                            continue;
                        }
                    }
                }
            }
    
            file.close();
        }

        void OfflineDumpAnalyzer::save_to_file(const std::string& filename, SaveFormat format) {
            std::ofstream file(filename, std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open file for writing: " + filename);
            }
            
            switch (format) {
                case SaveFormat::Binary: {
                    cereal::BinaryOutputArchive archive(file);
                    archive(addr2inst, addr2func_name, func_name2insts);
                    break;
                }
                case SaveFormat::JSON: {
                    cereal::JSONOutputArchive archive(file);
                    archive(addr2inst, addr2func_name, func_name2insts);
                    break;
                }
                case SaveFormat::XML: {
                    cereal::XMLOutputArchive archive(file);
                    archive(addr2inst, addr2func_name, func_name2insts);
                    break;
                }
            }
            
            file.close();
        }

        void OnlineDumpAnalyzer::load_from_file(const std::string& filename, SaveFormat format) {
            std::ifstream file(filename, std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open file for reading: " + filename);
            }
            
            clear_data();

            switch (format) {
                case SaveFormat::Binary: {
                    cereal::BinaryInputArchive archive(file);
                    archive(addr2inst, addr2func_name, func_name2insts);
                    break;
                }
                case SaveFormat::JSON: {
                    cereal::JSONInputArchive archive(file);
                    archive(addr2inst, addr2func_name, func_name2insts);
                    break;
                }
                case SaveFormat::XML: {
                    cereal::XMLInputArchive archive(file);
                    archive(addr2inst, addr2func_name, func_name2insts);
                    break;
                }
            }
            
            file.close();
        }

        OnlineDumpAnalyzer::OnlineDumpAnalyzer(const uint64_t base_addr){
            this->base_addr = base_addr;
        }

        OnlineDumpAnalyzer& OnlineDumpAnalyzer::getInstance(const uint64_t base_addr){
            static OnlineDumpAnalyzer instance(base_addr);
            return instance;
        }

        uint64_t OnlineDumpAnalyzer::to_abs(uint64_t rela_addr){
            return base_addr + rela_addr;
        }

        uint64_t OnlineDumpAnalyzer::to_rela(uint64_t abs_addr){
            return abs_addr - base_addr;
        }

        std::vector<Instruction*> OnlineDumpAnalyzer::select_func_content(uint64_t addr_inside_abs){
            uint64_t addr_inside = to_rela(addr_inside_abs);
            std::string func_name = addr2func_name[addr_inside];
            auto& insts_shared_ptrs = func_name2insts[func_name];
            std::vector<Instruction*> insts_raw_ptrs;
            for (const auto& inst_shared_ptr : insts_shared_ptrs) {
                insts_raw_ptrs.push_back(inst_shared_ptr.get());
            }
            return insts_raw_ptrs;
        }

        Instruction* OnlineDumpAnalyzer::addr_to_inst(uint64_t addr_abs){
            uint64_t addr = to_rela(addr_abs);
            if (addr2inst.find(addr) != addr2inst.end()) {
                return addr2inst[addr].get();
            }
            return nullptr;
        }

        std::vector<uint64_t> OnlineDumpAnalyzer::insts_to_abs_addrs(const std::vector<Instruction*>& insts) {
            std::vector<uint64_t> abs_addrs;
            for (const auto& inst : insts) {
                abs_addrs.push_back(to_abs(inst->address));
            }
            return abs_addrs;
        }

    } // namespace Dump
} // namespace BinaryTranslation