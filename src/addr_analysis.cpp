#include "addr_analysis.h"

namespace AddrAnalysis {

bool is_vector_assignment(const std::string& mnemonic) {
    // Check if instruction is a vector assignment (writes to vector register)
    return mnemonic == "vlw.v" || 
           mnemonic == "vsw.v" ||
           mnemonic == "vmv.v.v" ||
           mnemonic == "vadd.vv" ||
           mnemonic == "vsub.vv" ||
           mnemonic == "vmul.vv" ||
           mnemonic == "vdiv.vv" ||
           mnemonic == "vsetvli";
}

bool is_vector_instruction(const std::string& mnemonic) {
    // Check if instruction uses vector registers (mnemonic starts with 'v')
    return mnemonic.find("v") == 0;
}

std::vector<int> parse_vector_operands(const std::string& operands) {
    // Parse vector operands and extract register numbers
    std::vector<int> reg_nums;
    
    if (operands.empty()) {
        return reg_nums;
    }
    
    std::stringstream ss(operands);
    std::string operand;
    
    while (std::getline(ss, operand, ',')) {
        // Trim whitespace
        operand.erase(0, operand.find_first_not_of(" \t"));
        operand.erase(operand.find_last_not_of(" \t") + 1);
        
        // Check if operand is a vector register
        if (operand.find("v") == 0) {
            try {
                std::string reg_str = operand.substr(1);
                int reg_num = std::stoi(reg_str);
                if (reg_num >= 0 && reg_num < 32) {
                    reg_nums.push_back(reg_num);
                }
            } catch (const std::exception& e) {
                // Ignore parsing errors
            }
        }
    }
    
    return reg_nums;
}

void propagate_source_through_blocks(Source* source, VectorInst* source_inst, CodeBlock* start_block, 
                                   std::vector<CodeBlock*>& code_blocks,
                                   std::vector<Source*>& sources,
                                   std::map<uint64_t, VectorInst*>& insts) {
    // DFS traversal of basic blocks
    std::stack<CodeBlock*> worklist;
    std::set<uint64_t> visited;  // Track visited instruction addresses
    worklist.push(start_block);
    Instruction *instr_ptr = start_block->instructions[0];
    while(instr_ptr->address != source_inst->address) {
        instr_ptr++;
    }
    instr_ptr++;
    uint64_t current_addr = std::stoull(instr_ptr->address, nullptr, 16);
    while (!worklist.empty()) {
        CodeBlock* current_block = worklist.top();
        worklist.pop();
        
        if (current_addr != std::stoull(source_inst->address, nullptr, 16) + source_inst->inst_size) {
            instr_ptr = current_block->instructions[0];
            current_addr = std::stoull(instr_ptr->address, nullptr, 16);
        }

        if (visited.count(current_addr)) {
            continue;
        }
        visited.insert(current_addr);

        uint64_t block_end = std::stoull(current_block->endaddr, nullptr, 16);
        bool path_ended_by_source = false;  // Flag to track if path ended due to source instruction
        

        while (current_addr < block_end) {
            // Check if this instruction creates a new source (path ends)
            bool is_source_instruction = false;
            for (Source* other_source : sources) {
                if (other_source->inst_addr == current_addr) {
                    is_source_instruction = true;
                    break;
                }
            }
            // If this is a source instruction, don't access it and stop this path
            if (is_source_instruction) {
                path_ended_by_source = true;  // Set flag
                break; // Stop scanning this path
            }
            
            // Check if this instruction is in insts
            auto vec_inst_it = insts.find(current_addr);
            if (vec_inst_it != insts.end()) {
                VectorInst* vec_inst = vec_inst_it->second;
                
                // Check if this instruction uses the target register
                // Use reg_sources to see if this register is used in this instruction
                auto reg_it = vec_inst->reg_sources.find(source->target_reg);
                if (reg_it != vec_inst->reg_sources.end()) {
                    // Set source to this register (overwrite if exists)
                    vec_inst->reg_sources[source->target_reg] = source;
                }
            }
            // Move to next instruction
            instr_ptr++;
            current_addr = std::stoull(instr_ptr->address, nullptr, 16);
        }
        
        // Add successor blocks to worklist only if path didn't end due to source instruction
        if (!path_ended_by_source) {
            for (const auto& jump_target : current_block->jumpto) {
                // Find block with this start address
                for (CodeBlock* block : code_blocks) {
                    if (block->startaddr == jump_target) {
                        worklist.push(block);
                        break;
                    }
                }
            }
        }
    }
}

void create_vector_instruction(Instruction* instr, const std::string& mnemonic, 
                                CodeBlock* block, const std::vector<int>& reg_nums,
                                std::vector<Source*>& sources, std::map<uint64_t, VectorInst*>& insts) {
    // Create vector instruction and handle source creation if needed
    VectorInst* vec_inst = new VectorInst(instr->address, instr->instrlen, mnemonic, block, reg_nums);
    if (is_vector_assignment(mnemonic)) {
        int target_reg = 0;
        if (!reg_nums.empty()) {
            target_reg = reg_nums[0]; // First register is target
        }
        Source* source = new Source(instr->address, target_reg);
        sources.push_back(source);
        vec_inst->reg_sources[target_reg] = source;
    }
    insts[instr->address] = vec_inst;
}

void init_sources_insts(std::vector<CodeBlock*>& code_blocks,
                       std::vector<Source*>& sources, 
                       std::map<uint64_t, VectorInst*>& insts) {
    // Initialize sources and instructions by scanning all code blocks
    for (CodeBlock* block : code_blocks) {
        // Process instructions in this block
        for (Instruction* instr : block->instructions) {
            std::string mnemonic = instr->opcode;
            
            if (is_vector_instruction(mnemonic)) {
                std::vector<int> reg_nums = parse_vector_operands(instr->operand);
                create_vector_instruction(instr, mnemonic, block, reg_nums, sources, insts);
            }
        }
    }
}

void tag_sources(std::vector<CodeBlock*>& code_blocks,
                std::vector<Source*>& sources, 
                std::map<uint64_t, VectorInst*>& insts) {
    for (Source* source : sources) {
        VectorInst* source_inst = insts[source->inst_addr];
        if (!source_inst) continue;
        
        CodeBlock* source_block = source_inst->parent_block;
        if (!source_block) continue;
        
        // Propagate source through reachable code blocks
        propagate_source_through_blocks(source, source_inst, source_block, code_blocks, sources, insts);
    }
}

int count_unknown_sources(std::vector<Source*>& sources) {
    // Count sources with unknown attributes
    int count = 0;
    for (Source* source : sources) {
        if (source->attrib == SourceAttrib::UNKNOWN) {
            count++;
        }
    }
    return count;
}

void analyze_source_bit_width(std::map<uint64_t, VectorInst*>& insts) {
    for (auto& pair : insts) {
        VectorInst* vec_inst = pair.second;
        
        // Check each register's source
        for (auto& reg_pair : vec_inst->reg_sources) {
            Source* reg_source = reg_pair.second;
            
            // Check if this register has empty source or 1024 source
            bool has_empty_or_1024 = false;
            if(reg_source == nullptr || reg_source->attrib == SourceAttrib::BIT_1024) {
                has_empty_or_1024 = true;
            }
            
            if(has_empty_or_1024){
                for (auto& inner_reg_pair : vec_inst->reg_sources) {
                    if (inner_reg_pair.second) {
                        inner_reg_pair.second->attrib = SourceAttrib::BIT_1024;
                    }
                }
            }
        }
    }
}

void judge_sources(std::vector<Source*>& sources, 
                 std::map<uint64_t, VectorInst*>& insts) {
    while(true){
        int unknown_count_before = count_unknown_sources(sources);
        analyze_source_bit_width(insts);
        int unknown_count_after = count_unknown_sources(sources);
        if(unknown_count_before == unknown_count_after){
            break;
        }
    }
}

bool needs_translation(VectorInst* vec_inst) {
    // Check if instruction needs translation based on register sources
    for (auto& reg_pair : vec_inst->reg_sources) {
        Source* source = reg_pair.second;
        if (source == nullptr || (source && source->attrib == SourceAttrib::BIT_1024)) {
            return true;
        }
    }
    return false;
}

std::vector<std::pair<uint64_t, uint64_t>> group_consecutive_addresses(
    const std::set<uint64_t>& translation_addrs,
    std::map<uint64_t, VectorInst*>& insts) {
    // Group consecutive addresses into ranges using actual instruction lengths
    std::vector<std::pair<uint64_t, uint64_t>> ranges;
    
    if (translation_addrs.empty()) {
        return ranges;
    }
    
    auto it = translation_addrs.begin();
    uint64_t range_start = *it;
    uint64_t prev_addr = *it;
    uint64_t prev_end = prev_addr + insts[prev_addr]->inst_size;
    ++it;
    
    for (; it != translation_addrs.end(); ++it) {
        uint64_t current_addr = *it;
        
        // If current address is not consecutive with previous, end current range
        if (current_addr != prev_end) {
            ranges.push_back(std::make_pair(range_start, prev_end));
            range_start = current_addr;
        }
        
        prev_addr = current_addr;
        prev_end = prev_addr + insts[prev_addr]->inst_size;
    }
    
    // Add the final range
    ranges.push_back(std::make_pair(range_start, prev_end));
    return ranges;
}

std::vector<std::pair<uint64_t, uint64_t>> get_ranges(
    std::vector<Source*>& sources, 
    std::map<uint64_t, VectorInst*>& insts) {
    // Generate translation ranges based on instruction scanning
    std::set<uint64_t> translation_addrs;
    
    // Scan all instructions to find those that need translation
    for (auto& pair : insts) {
        VectorInst* vec_inst = pair.second;
        if (needs_translation(vec_inst)) {
            translation_addrs.insert(vec_inst->inst_addr);
        }
    }
    
    return group_consecutive_addresses(translation_addrs, insts);
}

std::vector<std::pair<uint64_t, uint64_t>> analyze_vector_register_binary(std::vector<CodeBlock*>& code_blocks) {
    // Main analysis function using binary-based algorithm
    if (code_blocks.empty()) {
        return std::vector<std::pair<uint64_t, uint64_t>>();
    }
    
    std::vector<Source*> sources;
    std::map<uint64_t, VectorInst*> insts;
    
    // Initialize sources and instructions
    init_sources_insts(code_blocks, sources, insts);
    
    // Tag sources
    tag_sources(code_blocks, sources, insts);
    
    // Judge source attributes
    judge_sources(sources, insts);
    
    // Generate translation ranges
    return get_ranges(sources, insts);
}

} // namespace AddrAnalysis
