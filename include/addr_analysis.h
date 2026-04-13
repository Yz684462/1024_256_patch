#ifndef RVMIG_ADDR_ANALYSIS_H
#define RVMIG_ADDR_ANALYSIS_H

#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <stack>
#include <set>
#include <iostream>
#include "globals.h"
#include "config.h"
#include "binary.h"

// Source attribute enumeration
enum class SourceAttrib {
    UNKNOWN,   // Unknown attribute
    BIT_1024,  // 1024-bit
    BIT_256    // 256-bit
};

// Source structure - represents a register source
struct Source {
    uint64_t inst_addr;           // Instruction address where source is created
    int target_reg;               // Target register number (0-31)
    SourceAttrib attrib;          // Source attribute
    
    Source(uint64_t addr, int reg) 
        : inst_addr(addr), target_reg(reg), attrib(SourceAttrib::UNKNOWN) {}
};

// Vector instruction structure
struct VectorInst {
    uint64_t inst_addr;           // Instruction address
    int inst_size;                // Instruction length
    std::string mnemonic;          // Instruction mnemonic
    CodeBlock* parent_block;      // Parent basic block pointer
    std::vector<int> operands;    // Register operands as numbers
    std::map<int, Source*> reg_sources;  // Register to source mapping
    
    VectorInst(uint64_t addr, int size, const std::string& mnemonic, 
                CodeBlock* block, const std::vector<int>& regs)
        : inst_addr(addr), inst_size(size), mnemonic(mnemonic), 
          parent_block(block), operands(regs) {}
};

namespace AddrAnalysis {

// Function declarations
bool is_vector_assignment(const std::string& mnemonic);
bool is_vector_instruction(const std::string& mnemonic);

void init_sources_insts(std::vector<CodeBlock*>& code_blocks,
                       std::vector<Source*>& sources, 
                       std::map<uint64_t, VectorInst*>& insts);

void tag_sources(std::vector<CodeBlock*>& code_blocks,
                std::vector<Source*>& sources, 
                std::map<uint64_t, VectorInst*>& insts);

void judge_sources(std::vector<Source*>& sources, 
                 std::map<uint64_t, VectorInst*>& insts);

std::vector<std::pair<uint64_t, uint64_t>> get_ranges(
    std::vector<Source*>& sources, 
    std::map<uint64_t, VectorInst*>& insts);

std::vector<std::pair<uint64_t, uint64_t>> analyze_vector_register_binary(std::vector<CodeBlock*>& code_blocks);

} // namespace AddrAnalysis

#endif // RVMIG_ADDR_ANALYSIS_H
