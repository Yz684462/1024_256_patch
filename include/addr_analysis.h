#ifndef ADDR_ANALYSIS_H
#define ADDR_ANALYSIS_H

#include <cstdint>
#include <vector>
#include <string>
#include <set>

// Forward declarations
struct CFG;
struct CFGBlock;
struct Instruction;

// Vector analysis state
struct VectorAnalysisState {
    uint64_t pc_migration;
    std::vector<int> reg_attrs;           // Register attributes (1024=valid, 256=pending, other=undefined)
    std::vector<std::vector<uint64_t>> reg_queues; // Assignment instruction PC queues
    std::vector<uint64_t> simulation_instructions; // Instructions needing simulation
    CFG* cfg;                           // Control flow graph reference
};

// Register usage analysis result
struct RegisterUsage {
    bool has_1024;
    bool has_pending;
};

namespace AddrAnalysis {

// Main analysis function - returns translation ranges
std::vector<std::pair<uint64_t, uint64_t>> analyze_vector_register(uint64_t func_addr, CFG *cfg);

// Initialize analysis state
VectorAnalysisState initialize_analysis_state(uint64_t func_addr);

// Scan all instructions in CFG
void scan_instructions(CFG *cfg, VectorAnalysisState& state);

// Process individual instruction
void process_instruction(const Instruction& instr, VectorAnalysisState& state);

// Check if instruction is vector assignment
bool is_vector_assignment(const Instruction& instr);

// Process assignment instruction
void process_assignment_instruction(const Instruction& instr, uint64_t pc, VectorAnalysisState& state);

// Process non-assignment instruction
void process_non_assignment_instruction(const Instruction& instr, uint64_t pc, VectorAnalysisState& state);

// Analyze register usage in instruction
RegisterUsage analyze_register_usage(const Instruction& instr, const VectorAnalysisState& state);

// Set pending registers to 1024-bit
void set_pending_registers_to_1024(const Instruction& instr, VectorAnalysisState& state);

// Parse target register from instruction
int parse_target_register(const Instruction& instr);

// Parse register number from string
int parse_register_number(const std::string& reg_str);

// Execute modify operations
void execute_modify_operations(int target_reg, uint64_t start_pc, VectorAnalysisState& state);

// Find block by PC
const CFGBlock* find_block_by_pc(CFG *cfg, uint64_t pc);

// Find block by address
const CFGBlock* find_block_by_addr(CFG *cfg, uint64_t addr);

// Scan block instructions in reverse
void scan_block_instructions_reverse(const CFGBlock& block, uint64_t start_pc, uint64_t pc_migration, 
                                int target_reg, VectorAnalysisState& state,
                                std::vector<uint64_t>& worklist, std::set<uint64_t>& visited_blocks);

// Check if instruction contains register
bool instruction_contains_register(const Instruction& instr, int target_reg);

// Process register dependencies
void process_register_dependencies(const Instruction& instr, int target_reg, uint64_t pc, VectorAnalysisState& state);

} // namespace AddrAnalysis

#endif // ADDR_ANALYSIS_H
