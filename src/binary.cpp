#include "binary.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <fstream>
#include <sstream>
#include <algorithm>

// Instruction type definitions
const std::vector<std::string> jmp_instr = {"j", "jal"};
const std::vector<std::string> branch_instr = {"beq", "bne", "beqz", "bnez", "blt", "bge", "bltz", "bgez", "bltu", "bgeu", "blez", "bgtz"};
const std::vector<std::string> return_instr = {"ret"};
const std::vector<std::string> jmp_indirect_instr = {"jalr", "jr"};
const std::vector<std::string> other_instr = {"ebreak"};

// Forward declarations
class Instruction;
class CodeBlock;

// Utility functions
std::vector<std::string> extractop(const std::string& operand) {
    std::vector<std::string> operandlist;
    std::stringstream ss(operand);
    std::string item;
    
    while (std::getline(ss, item, ',')) {
        // Remove leading and trailing whitespace
        item.erase(0, item.find_first_not_of(" \t"));
        item.erase(item.find_last_not_of(" \t") + 1);
        operandlist.push_back(item);
    }
    
    return operandlist;
}

bool contains(const std::vector<std::string>& vec, const std::string& str) {
    return std::find(vec.begin(), vec.end(), str) != vec.end();
}

class Instruction {
public:
    std::string address;
    std::string opcode;
    std::string operand;
    std::vector<std::string> operand_extract;
    std::string machine_code;
    int instrlen;
    std::vector<std::string> jumpto;
    std::vector<std::string> jumpfrom;
    bool isblockbegin;
    bool isblockend;
    bool isret;

    Instruction(const std::string& opcode, const std::string& operand, 
                const std::string& address = "0x0000", const std::string& machine_code = "0000")
        : address(address), opcode(opcode), machine_code(machine_code), instrlen(0), 
          isblockbegin(false), isblockend(false), isret(false) {
        
        // Remove content after '<' from operand
        size_t pos = operand.find('<');
        this->operand = (pos != std::string::npos) ? operand.substr(0, pos) : operand;
        
        this->operand_extract = extractop(this->operand);
        this->instrlen = machine_code.length() / 2;
    }

    std::string toString() const {
        return opcode + " " + operand;
    }

    std::string repr() const {
        std::stringstream ss;
        ss << "Instruction(address='" << address << "', opcode='" << opcode << "', "
           << "operand='" << operand << "', machine_code='" << machine_code << "', "
           << "instrlen='" << instrlen << "', jumpto=";
        
        for (const auto& addr : jumpto) {
            ss << "'" << addr << "',";
        }
        
        ss << "operand_extract=";
        for (const auto& op : operand_extract) {
            ss << "'" << op << "',";
        }
        
        ss << "jumpfrom=";
        for (const auto& addr : jumpfrom) {
            ss << "'" << addr << "',";
        }
        
        ss << ")";
        return ss.str();
    }
};

void update_jump_aim(std::map<std::string, Instruction*>& instructions, std::vector<std::pair<std::string, std::string>>& erraddr) {
    for (auto& pair : instructions) {
        Instruction* i = pair.second;
        if (i->isblockend) {
            for (const auto& j : i->jumpto) {
                if (instructions.find(j) == instructions.end()) {
                    erraddr.push_back({i->address, j});
                } else {
                    instructions[j]->jumpfrom.push_back(i->address);
                    instructions[j]->isblockbegin = true;
                }
            }
        }
    }
}

void addjumpto_target(Instruction* instr) {
    if (contains(jmp_instr, instr->opcode)) {
        std::stringstream ss(instr->operand);
        std::string first_part;
        ss >> first_part;
        instr->jumpto.push_back("0x" + first_part);
    } else if (contains(branch_instr, instr->opcode)) {
        std::stringstream ss(instr->operand);
        std::string last_part;
        std::string part;
        while (ss >> part) {
            last_part = part;
        }
        instr->jumpto.push_back("0x" + last_part);
    }
}

void identify_blockend(Instruction* instr) {
    if (contains(jmp_instr, instr->opcode) || contains(branch_instr, instr->opcode)) {
        instr->isblockend = true;
        addjumpto_target(instr);
    } else if (contains(jmp_indirect_instr, instr->opcode) || 
               contains(return_instr, instr->opcode) || 
               contains(other_instr, instr->opcode)) {
        instr->isblockend = true;
    }
}

std::pair<std::map<std::string, Instruction*>, std::vector<std::pair<std::string, std::string>>> 
parse_objdump_output(const std::string& dump_content) {
    std::map<std::string, Instruction*> instructions;
    std::vector<std::pair<std::string, std::string>> erraddr;
    std::stringstream ss(dump_content);
    std::string line;
    
    std::regex pattern(R"(^\s*([0-9a-fA-F]+):\s+([0-9a-fA-F]{4,8})\s+([\w.]+)\s*(.*)$)");
    
    while (std::getline(ss, line)) {
        std::smatch match;
        if (std::regex_match(line, match, pattern)) {
            std::string address = "0x" + match[1].str();
            std::string machine_code = match[2].str();
            // Remove spaces from machine code and convert to lowercase
            machine_code.erase(std::remove(machine_code.begin(), machine_code.end(), ' '), machine_code.end());
            std::transform(machine_code.begin(), machine_code.end(), machine_code.begin(), ::tolower);
            
            std::string opcode = match[3].str();
            std::string operand_str = match[4].str();
            
            // Remove comments after # or <
            size_t pos = operand_str.find('#');
            if (pos != std::string::npos) {
                operand_str = operand_str.substr(0, pos);
            }
            pos = operand_str.find('<');
            if (pos != std::string::npos) {
                operand_str = operand_str.substr(0, pos);
            }
            operand_str.erase(0, operand_str.find_first_not_of(" \t"));
            operand_str.erase(operand_str.find_last_not_of(" \t") + 1);
            
            Instruction* i = new Instruction(opcode, operand_str, address, machine_code);
            identify_blockend(i);
            instructions[address] = i;
        }
    }
    
    update_jump_aim(instructions, erraddr);
    return {instructions, erraddr};
}

std::string extract_func_contain_addr(const std::string& dump_content, const std::string& addr) {
    std::stringstream ss(dump_content);
    std::string line;
    std::vector<std::string> lines;
    
    // Store all lines
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
    
    // Convert target address to hex format for comparison
    std::string target_addr = addr;
    if (target_addr.substr(0, 2) == "0x") {
        target_addr = target_addr.substr(2);
    }
    std::transform(target_addr.begin(), target_addr.end(), target_addr.begin(), ::tolower);
    
    int prev_func_line_num = -1;
    int cur_func_line_num = -1;
    
    std::regex func_pattern(R"(^\s*([0-9a-fA-F]+)\s+<.*>.*:$)");
    
    for (int line_num = 0; line_num < lines.size(); line_num++) {
        std::smatch match;
        if (std::regex_match(lines[line_num], match, func_pattern)) {
            prev_func_line_num = cur_func_line_num;
            cur_func_line_num = line_num;
            std::string func_addr = match[1].str();
            std::transform(func_addr.begin(), func_addr.end(), func_addr.begin(), ::tolower);
            
            // If function address is greater than target address, stop searching
            unsigned long func_addr_val = std::stoul(func_addr, nullptr, 16);
            unsigned long target_addr_val = std::stoul(target_addr, nullptr, 16);
            if (func_addr_val > target_addr_val) {
                break;
            }
        }
    }
    
    // Extract target function content
    std::string result;
    if (prev_func_line_num >= 0) {
        for (int i = prev_func_line_num + 1; i < cur_func_line_num; i++) {
            result += lines[i] + "\n";
        }
    }
    
    return result;
}

std::pair<std::vector<Instruction*>, std::vector<std::pair<std::string, std::string>>> 
disam_binary_with_dump_func_contain_addr(const std::string& dump_path, const std::string& addr) {
    std::vector<Instruction*> instructions;
    
    std::ifstream file(dump_path);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << dump_path << std::endl;
        return {instructions, {}};
    }
    
    std::string output((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    output = extract_func_contain_addr(output, addr);
    auto [instructions_dict, erraddr] = parse_objdump_output(output);
    
    // Convert map to vector
    for (auto& pair : instructions_dict) {
        instructions.push_back(pair.second);
    }
    
    std::cout << "erraddr count: " << erraddr.size() << std::endl;
    return {instructions, erraddr};
}

class CodeBlock {
public:
    int index;
    std::vector<Instruction*> instructions;
    std::string startaddr;
    std::string endaddr;
    std::vector<int> addrange;
    std::vector<std::string> jumpto;
    std::vector<std::string> jumpfrom;
    int instnum;
    bool retblock;
    bool hasExtInstr;
    bool hasIndirectJump;

    CodeBlock(const std::vector<Instruction*>& instructions, int index = 0)
        : index(index), instructions(instructions), instnum(instructions.size()), 
          retblock(false), hasExtInstr(false), hasIndirectJump(false) {
        
        if (!instructions.empty()) {
            startaddr = instructions[0]->address;
            endaddr = instructions.back()->address;
            addrange = {static_cast<int>(std::stoul(startaddr, nullptr, 16)), 
                       static_cast<int>(std::stoul(endaddr, nullptr, 16))};
            jumpto = instructions.back()->jumpto;
            jumpfrom = instructions[0]->jumpfrom;
            
            if (instructions.back()->isret) {
                retblock = true;
            }
        }
    }

    std::string toString() const {
        std::stringstream ss;
        for (const auto& instr : instructions) {
            ss << instr->toString() << "\n";
        }
        return ss.str();
    }

    std::string repr() const {
        std::stringstream ss;
        ss << "CodeBlock(startaddr='" << startaddr << "', endaddr='" << endaddr 
           << "', jumpto='" << (jumpto.empty() ? "" : jumpto[0]) 
           << "', instr_num='" << instnum << "')";
        return ss.str();
    }
};

std::vector<CodeBlock*> get_codeblocks_linear(const std::vector<Instruction*>& instructions) {
    std::vector<CodeBlock*> codeblocks;
    
    int i = 0;
    int j = 0;
    std::vector<std::vector<Instruction*>> blocks;
    
    while (j < instructions.size()) {
        if (instructions[j]->isblockbegin && i != j) {
            blocks.push_back(std::vector<Instruction*>(instructions.begin() + i, instructions.begin() + j));
            i = j;
        }
        if (instructions[j]->isblockend) {
            blocks.push_back(std::vector<Instruction*>(instructions.begin() + i, instructions.begin() + j + 1));
            i = j + 1;
            j++;
        } else {
            j++;
        }
    }
    
    if (i != j) {
        blocks.push_back(std::vector<Instruction*>(instructions.begin() + i, instructions.begin() + j));
    }
    
    int idx = 0;
    for (const auto& ins : blocks) {
        codeblocks.push_back(new CodeBlock(ins, idx));
        idx++;
    }
    
    // Add the last instruction executed in sequence to the jumpfrom field of block
    std::vector<std::string> all_jmp_instr;
    all_jmp_instr.insert(all_jmp_instr.end(), jmp_instr.begin(), jmp_instr.end());
    all_jmp_instr.insert(all_jmp_instr.end(), branch_instr.begin(), branch_instr.end());
    all_jmp_instr.insert(all_jmp_instr.end(), return_instr.begin(), return_instr.end());
    all_jmp_instr.insert(all_jmp_instr.end(), jmp_indirect_instr.begin(), jmp_indirect_instr.end());
    
    for (int idx = 1; idx < codeblocks.size(); idx++) {
        Instruction* i = codeblocks[idx]->instructions[0];
        std::string preAddr = codeblocks[idx-1]->endaddr;
        std::string preInstrOp = codeblocks[idx-1]->instructions.back()->opcode;
        
        if (!contains(all_jmp_instr, preInstrOp) && preInstrOp.find("...") == std::string::npos) {
            i->jumpfrom.push_back(preAddr);
        }
    }
    
    return codeblocks;
}

class Binary {
public:
    std::string dump_path;
    std::string addr;
    std::vector<Instruction*> instructions;
    std::vector<std::pair<std::string, std::string>> erradd;
    std::vector<CodeBlock*> code_blocks;

    Binary(const std::string& dump_path, const std::string& addr) 
        : dump_path(dump_path), addr(addr) {
        
        auto [instrs, errs] = disam_binary_with_dump_func_contain_addr(dump_path, addr);
        instructions = instrs;
        erradd = errs;
        code_blocks = get_codeblocks_linear(instructions);
    }

    ~Binary() {
        // Clean up allocated memory
        for (auto* instr : instructions) {
            delete instr;
        }
        for (auto* block : code_blocks) {
            delete block;
        }
    }
};
