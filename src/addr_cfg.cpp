#include "addr_cfg.h"
#include "../include/globals.h"
#include "../include/config.h"
#include <r_core.h>
#include <r_anal.h>

namespace AddrCFG {

CFG* build_cfg(RAnalFunction *func, RCore *core) {
    CFG *cfg = new CFG();
    
    if (!func) {
        return cfg;
    }
    
    printf("Found function: %s at 0x%lx, size = %lu\n", 
           func->name, func->addr, r_anal_function_linear_size(func));
    
    // Get basic blocks
    RList *blocks = func->bbs;
    RListIter *iter;
    void *ptr;
    
    // Find entry block (first block)
    RAnalBlock *entry_block = nullptr;
    
    r_list_foreach(blocks, iter, ptr) {
        RAnalBlock *block = reinterpret_cast<RAnalBlock*>(ptr);
        
        // Create CFG block
        CFGBlock cfg_block = create_cfg_block(block, core);
        
        cfg->blocks.push_back(cfg_block);
        
        // Set entry block (first block found)
        if (!entry_block) {
            entry_block = block;
            cfg->entry_block_addr = block->addr;
        }
        
        printf("  Block @ 0x%lx (size=%lu, instr_count=%lu)\n", 
               block->addr, block->size, cfg_block.instructions.size());
    }
    
    // Build predecessor relationships
    build_predecessor_relationships(cfg);
    
    return cfg;
}

CFGBlock create_cfg_block(RAnalBlock *block, RCore *core) {
    CFGBlock cfg_block;
    cfg_block.addr = block->addr;
    cfg_block.size = block->size;
    
    // Get instructions in this block
    for (int i = 0; i < block->ninstr; ++i) {
        ut64 instr_addr = block->addr + r_anal_bb_offset_inst(block, i);
        
        // Get instruction details
        RAnalOp *op = r_core_anal_op(core, instr_addr, R_ARCH_OP_MASK_VAL);
        if (op && op->mnemonic) {
            Instruction instr = create_instruction(op, instr_addr);
            cfg_block.instructions.push_back(instr);
            r_anal_op_free(op);
        }
    }
    
    // Get successors
    if (block->jump != UT64_MAX) {
        cfg_block.successors.push_back(block->jump);
    }
    if (block->fail != UT64_MAX) {
        cfg_block.successors.push_back(block->fail);
    }
    
    return cfg_block;
}

Instruction create_instruction(RAnalOp *op, uint64_t addr) {
    Instruction instr;
    instr.addr = addr;
    instr.mnemonic = std::string(op->mnemonic);
    
    // Get operands
    for (int j = 0; j < op->n_op; ++j) {
        if (op->ops[j].type != R_ANAL_OP_TYPE_NULL) {
            instr.operands.push_back(std::string(op->ops[j].buf));
        }
    }
    
    return instr;
}

void build_predecessor_relationships(CFG *cfg) {
    for (auto& block : cfg->blocks) {
        for (uint64_t succ_addr : block.successors) {
            // Find successor block and add current block as predecessor
            for (auto& succ_block : cfg->blocks) {
                if (succ_block.addr == succ_addr) {
                    succ_block.predecessors.push_back(block.addr);
                    break;
                }
            }
        }
    }
}

} // namespace AddrCFG
