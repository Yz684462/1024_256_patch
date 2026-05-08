#ifndef TYPES_H
#define TYPES_H

#include <vector>
#include <string>
#include <cstdint>

namespace BinaryTranslation {
    class Instruction {
        public:
            uint64_t address;
            std::string opcode;
            std::vector<std::string> operands;
            int instrlen;
            std::vector<uint64_t> jumpto;
            std::vector<uint64_t> jumpfrom;
            bool isblockbegin;
            bool isblockend;
            bool isret;
            std::string line;

            // 默认构造函数（Cereal 需要）
            Instruction() : address(0), instrlen(0), isblockbegin(false), 
                            isblockend(false), isret(false) {}
            
            Instruction(const std::string& line, const std::string& opcode, const std::string& operand, 
                        uint64_t address = 0x0000, int instrlen = 0);
            
            // Cereal 序列化方法
            template<class Archive>
            void serialize(Archive& archive) {
                archive(address, opcode, operands, instrlen, jumpto, jumpfrom, 
                        isblockbegin, isblockend, isret);
            }
    };

    class CodeBlock {
        public:
            std::vector<Instruction*> instructions;
            uint64_t startaddr;
            uint64_t endaddr;
            std::vector<uint64_t> jumpto;
            std::vector<uint64_t> jumpfrom;

            CodeBlock(const std::vector<Instruction*>& instructions);
    };

}

#endif // TYPES_H