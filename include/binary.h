#ifndef RVMIG_BINARY_H
#define RVMIG_BINARY_H

#include <string>
#include <vector>
#include <map>
#include <utility>

// Instruction type definitions
const std::vector<std::string> jmp_instr = {"j", "jal"};
const std::vector<std::string> branch_instr = {"beq", "bne", "beqz", "bnez", "blt", "bge", "bltz", "bgez", "bltu", "bgeu", "blez", "bgtz"};
const std::vector<std::string> return_instr = {"ret"};
const std::vector<std::string> jmp_indirect_instr = {"jalr", "jr"};
const std::vector<std::string> other_instr = {"ebreak"};

// Utility functions
std::vector<std::string> extractop(const std::string& operand);
bool contains(const std::vector<std::string>& vec, const std::string& str);

// Core classes
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
                const std::string& address = "0x0000", const std::string& machine_code = "0000");
    
    std::string toString() const;
    std::string repr() const;
};

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

    CodeBlock(const std::vector<Instruction*>& instructions, int index = 0);
    
    std::string toString() const;
    std::string repr() const;
};

class Binary {
public:
    std::string dump_path;
    std::string addr;
    std::vector<Instruction*> instructions;
    std::vector<std::pair<std::string, std::string>> erradd;
    std::vector<CodeBlock*> code_blocks;

    Binary(const std::string& dump_path, const std::string& addr);
    ~Binary();
};

// Core functions
std::pair<std::map<std::string, Instruction*>, std::vector<std::pair<std::string, std::string>>> 
parse_objdump_output(const std::string& dump_content);

std::string extract_func_contain_addr(const std::string& dump_content, const std::string& addr);

std::pair<std::vector<Instruction*>, std::vector<std::pair<std::string, std::string>>> 
disam_binary_with_dump_func_contain_addr(const std::string& dump_path, const std::string& addr);

std::vector<CodeBlock*> get_codeblocks_linear(const std::vector<Instruction*>& instructions);

#endif // RVMIG_BINARY_H
