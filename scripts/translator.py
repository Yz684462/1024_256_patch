#!/usr/bin/env python3
"""
RISC-V Vector Assembly Translator

This script translates RISC-V vector assembly code segments into C-compatible functions,
handling multiple address ranges and thread-specific offsets for simulated CPU state.
"""

import sys
import os
import re
import tempfile
from typing import Tuple

# Constants
VECTOR_CONTEXT_SIZE = 4192

# Setup path for rvv_sbt_tool
current_script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(current_script_path)
sys.path.append(os.path.join(script_dir, "..", "rvv_sbt_tool"))

from rvv_sbt import translate_function
from core.frontend.asm_parser import AsmParser


class AssemblyTranslator:
    """Handles translation of RISC-V assembly to C-compatible functions."""
    
    def __init__(self):
        self.translated_functions = []
    
    def translate_assembly(self, asm_content: str, func_name: str, translation_id: int, vtype:int) -> str:
        """Translate assembly content to C-compatible function."""
        parser = AsmParser()

        # Write assembly content to temporary file for parsing
        with tempfile.NamedTemporaryFile(mode='w', suffix='.s', delete=False) as f:
            f.write(asm_content)
            temp_file = f.name
        insn_list = parser.parse_file(temp_file)
        
        output = translate_function(
            insn_list,
            func_name=func_name,
            vlenb=128,
            source_file=temp_file,
            text_only=False,
            init_vtype=str(vtype),
        )

        return self._modify_translated_code(output, translation_id)
    
    def _modify_translated_code(self, translated: str, translation_id: int) -> str:
        """Modify translated code to handle simulated_cpu_state."""
        lines = translated.splitlines()
        modified_lines = []
        
        # Remove global simulated_cpu_state declarations
        for line in lines:
            if '.globl' in line and 'simulated_cpu_state' in line:
                continue
            if '.comm' in line and 'simulated_cpu_state' in line:
                continue
            if line.endswith(' ret'):
                modified_lines.append("\tebreak")
                continue
            modified_lines.append(line)
            if '.type' in line and f'translated_function_' in line:
                modified_lines.append(f'\t.extern simulated_cpu_state')

        
        translated = '\n'.join(modified_lines)
                
        offset = translation_id * VECTOR_CONTEXT_SIZE
        pattern = r'(\s+la\s+t6,\s+simulated_cpu_state)'
        if offset <= 2047:
            # Single addi instruction is sufficient
            replacement = f'\\1\n\taddi\tt6, t6, {offset}'
        else:
            # Need to use li + add sequence for large offsets
            replacement = f'\\1\n\tli\tt0, {offset}\n\tadd\tt6, t6, t0'
        translated = re.sub(pattern, replacement, translated)
        
        return translated


def parse_arguments() -> Tuple[int, str, str, int, str]:
    """Parse command line arguments."""
    if len(sys.argv) != 6:
        print("Usage: python translator.py <translation_id> <dump_line> <func_name> <vtype> <assembly_file>")
        print("Example: python translator.py 1 'dump line content' 'func1' 'vtype1' 'assembly_file'")
        print("Parameters:")
        print("  translation_id: Translation ID number")
        print("  dump_line: dump line to translate")
        print("  func_name: function name")
        print("  vtype: vector type")
        print("  assembly_file: output assembly_file path")
        sys.exit(1)
    
    try:
        translation_id = int(sys.argv[1])
        dump_line = sys.argv[2]
        func_name = sys.argv[3]
        vtype = int(sys.argv[4])
        assembly_file = sys.argv[5]
        return translation_id, dump_line, func_name, vtype, assembly_file

    except (ValueError, IndexError) as e:
        print(f"Error parsing arguments: {e}")
        sys.exit(1)


def main():
    """Main translation function."""
    # Parse arguments
    translation_id, dump_line, func_name, vtype, assembly_file = parse_arguments()
    
    translator = AssemblyTranslator()
    with open(assembly_file, 'a') as f:
        output = translator.translate_assembly(dump_line, func_name, translation_id, vtype)
        f.write("\n")
        f.write(output)
        f.write("\n")

if __name__ == "__main__":
    main()
