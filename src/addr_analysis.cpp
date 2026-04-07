#include "addr_analysis.h"
#include "../include/globals.h"
#include "../include/config.h"
#include <r_core.h>
#include <r_anal.h>

namespace AddrAnalysis {

std::vector<std::pair<uint64_t, uint64_t>> analyze_vector_register(uint64_t func_addr, CFG *cfg) {
    printf("[ANALYZE] Vector register analysis for function at 0x%lx\n", func_addr);
    
    if (!cfg) {
        return std::vector<std::pair<uint64_t, uint64_t>>();
    }
    
    // Initialize analysis state
    VectorAnalysisState state = initialize_analysis_state(func_addr);
    state.cfg = cfg;
    
    // Scan all instructions
    scan_instructions(cfg, state);
    
    // Convert simulation instructions to ranges
    std::vector<std::pair<uint64_t, uint64_t>> ranges;
    for (uint64_t pc : state.simulation_instructions) {
        // Find the basic block containing this PC
        for (const auto& block : cfg->blocks) {
            if (pc >= block.addr && pc < block.addr + block.size) {
                ranges.push_back(std::make_pair(block.addr, block.addr + block.size));
                break;
            }
        }
    }
    
    printf("[ANALYZE] Vector register analysis completed\n");
    printf("  Instructions needing simulation: %zu\n", state.simulation_instructions.size());
    printf("  Translation ranges generated: %zu\n", ranges.size());
    
    return ranges;
}

VectorAnalysisState initialize_analysis_state(uint64_t func_addr) {
    VectorAnalysisState state;
    state.pc_migration = func_addr;
    state.reg_attrs.resize(32, 1024); // All registers start as 1024-bit
    state.reg_queues.resize(32); // Initialize queues for each register
    
    return state;
}

void scan_instructions(CFG *cfg, VectorAnalysisState& state) {
    for (const auto& block : cfg->blocks) {
        for (const auto& instr : block.instructions) {
            process_instruction(instr, state);
        }
    }
}

void process_instruction(const Instruction& instr, VectorAnalysisState& state) {
    uint64_t current_pc = instr.addr;
    
    if (is_vector_assignment(instr)) {
        process_assignment_instruction(instr, current_pc, state);
    } else {
        process_non_assignment_instruction(instr, current_pc, state);
    }
}

bool is_vector_assignment(const Instruction& instr) {
    return instr.mnemonic.find("v") == 0 || 
           instr.mnemonic == "vlw.v" || 
           instr.mnemonic == "vsw.v" ||
           instr.mnemonic == "vmv.v.v" ||
           instr.mnemonic == "vadd.vv" ||
           instr.mnemonic == "vsub.vv" ||
           instr.mnemonic == "vmul.vv" ||
           instr.mnemonic == "vdiv.vv" ||
           instr.mnemonic == "vsetvli";
}

void process_assignment_instruction(const Instruction& instr, uint64_t pc, VectorAnalysisState& state) {
    int target_reg = parse_target_register(instr);
    
    if (target_reg < 0 || target_reg >= 32) {
        return;
    }
    
    // Handle pending attribute
    if (state.reg_attrs[target_reg] != 1024) {
        printf("    [MODIFY] Set reg v%d to 1024-bit at PC 0x%lx\n", target_reg, pc);
        state.reg_attrs[target_reg] = 1024;
    }
    
    // Mark as pending and add to queue
    state.reg_attrs[target_reg] = 256; // Pending
    state.reg_queues[target_reg].push_back(pc);
    
    // Execute modify operations
    execute_modify_operations(target_reg, pc, state);
}

void process_non_assignment_instruction(const Instruction& instr, uint64_t pc, VectorAnalysisState& state) {
    RegisterUsage usage = analyze_register_usage(instr, state);
    
    if (usage.has_1024 && usage.has_pending) {
        // Set pending registers to 1024-bit
        printf("    [MODIFY] Set pending regs to 1024-bit at PC 0x%lx\n", pc);
        set_pending_registers_to_1024(instr, state);
    }
    
    if (usage.has_1024) {
        state.simulation_instructions.push_back(pc);
    }
}

RegisterUsage analyze_register_usage(const Instruction& instr, const VectorAnalysisState& state) {
    RegisterUsage usage;
    
    for (const auto& operand : instr.operands) {
        if (operand.find("v") == 0) {
            int reg_num = parse_register_number(operand);
            if (reg_num >= 0 && reg_num < 32) {
                if (state.reg_attrs[reg_num] == 1024) {
                    usage.has_1024 = true;
                } else {
                    usage.has_pending = true;
                }
            }
        }
    }
    
    return usage;
}

void set_pending_registers_to_1024(const Instruction& instr, VectorAnalysisState& state) {
    for (const auto& operand : instr.operands) {
        if (operand.find("v") == 0) {
            int reg_num = parse_register_number(operand);
            if (reg_num >= 0 && reg_num < 32 && state.reg_attrs[reg_num] != 1024) {
                state.reg_attrs[reg_num] = 1024;
            }
        }
    }
}

int parse_target_register(const Instruction& instr) {
    if (instr.operands.empty()) {
        return -1;
    }
    
    // Try to extract register number from first operand
    for (const auto& operand : instr.operands) {
        if (operand.find("v") == 0) {
            return parse_register_number(operand);
        }
    }
    
    return -1;
}

int parse_register_number(const std::string& reg_str) {
    if (reg_str.empty() || reg_str[0] != 'v') {
        return -1;
    }
    
    try {
        std::string num_str = reg_str.substr(1);
        return std::stoi(num_str);
    } catch (...) {
        return -1;
    }
}

void execute_modify_operations(int target_reg, uint64_t start_pc, VectorAnalysisState& state) {
    std::set<uint64_t> visited_blocks;
    std::vector<uint64_t> worklist;
    
    // Find start block
    const CFGBlock* start_block = find_block_by_pc(state.cfg, start_pc);
    if (!start_block) {
        return;
    }
    
    worklist.push_back(start_block->addr);
    visited_blocks.insert(start_block->addr);
    
    // Process worklist
    while (!worklist.empty()) {
        uint64_t current_block_addr = worklist.back();
        worklist.pop_back();
        
        const CFGBlock* current_block = find_block_by_addr(state.cfg, current_block_addr);
        if (!current_block) continue;
        
        // Scan instructions in reverse
        scan_block_instructions_reverse(*current_block, start_pc, state.pc_migration, target_reg, state, worklist, visited_blocks);
    }
}

const CFGBlock* find_block_by_pc(CFG *cfg, uint64_t pc) {
    for (const auto& block : cfg->blocks) {
        if (pc >= block.addr && pc < block.addr + block.size) {
            return &block;
        }
    }
    return nullptr;
}

const CFGBlock* find_block_by_addr(CFG *cfg, uint64_t addr) {
    for (const auto& block : cfg->blocks) {
        if (block.addr == addr) {
            return &block;
        }
    }
    return nullptr;
}

void scan_block_instructions_reverse(const CFGBlock& block, uint64_t start_pc, uint64_t pc_migration, 
                                int target_reg, VectorAnalysisState& state,
                                std::vector<uint64_t>& worklist, std::set<uint64_t>& visited_blocks) {
    for (int64_t i = block.instructions.size() - 1; i >= 0; --i) {
        const Instruction& instr = block.instructions[i];
        uint64_t instr_pc = instr.addr;
        
        // Check instruction range
        if (instr_pc > start_pc) continue;
        if (instr_pc < pc_migration) break;
        
        // Check if instruction contains target register
        if (instruction_contains_register(instr, target_reg)) {
            process_register_dependencies(instr, target_reg, instr_pc, state);
        }
    }
    
    // Add predecessors to worklist
    for (uint64_t pred_addr : block.predecessors) {
        if (visited_blocks.find(pred_addr) == visited_blocks.end()) {
            visited_blocks.insert(pred_addr);
            worklist.push_back(pred_addr);
        }
    }
}

bool instruction_contains_register(const Instruction& instr, int target_reg) {
    for (const auto& operand : instr.operands) {
        if (operand.find("v") == 0) {
            int reg_num = parse_register_number(operand);
            if (reg_num == target_reg) {
                return true;
            }
        }
    }
    return false;
}

void process_register_dependencies(const Instruction& instr, int target_reg, uint64_t pc, VectorAnalysisState& state) {
    for (const auto& operand : instr.operands) {
        if (operand.find("v") == 0) {
            int reg_num = parse_register_number(operand);
            if (reg_num >= 0 && reg_num < 32 && state.reg_attrs[reg_num] != 1024) {
                printf("    [QUEUE] Add modify operation for reg v%d at PC 0x%lx\n", reg_num, pc);
                state.reg_queues[reg_num].push_back(pc);
            }
        }
    }
}

} // namespace AddrAnalysis
