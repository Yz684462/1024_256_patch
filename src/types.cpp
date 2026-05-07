#include "types.h"

#include <sstream>

namespace BinaryTranslation {

    Instruction::Instruction(const std::string& opcode, const std::string& operand, uint64_t address, int instrlen): 
        address(address), opcode(opcode), instrlen(instrlen), isblockbegin(false), isblockend(false), isret(false) {
        
        std::vector<std::string> operandlist;
        std::stringstream ss(operand);
        std::string item;
        
        while (std::getline(ss, item, ',')) {
            // Remove leading and trailing whitespace
            item.erase(0, item.find_first_not_of(" \t"));
            item.erase(item.find_last_not_of(" \t") + 1);
            operandlist.push_back(item);
        }
        this->operands = operandlist;
    }

    CodeBlock::CodeBlock(const std::vector<Instruction*>& instructions)
        : instructions(instructions) {
        
        if (!instructions.empty()) {
            startaddr = instructions[0]->address;
            endaddr = instructions.back()->address;
            jumpto = instructions.back()->jumpto;
            jumpfrom = instructions[0]->jumpfrom;
        }
    }
}