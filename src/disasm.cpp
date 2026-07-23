/**
 * @file disasm.cpp
 * @brief Bytecode disassembler implementation for human-readable output.
 *
 * Pretty-prints compiled bytecode with source line annotations and resolved
 * jump targets. Handles four instruction formats (simple, constant, byte, jump)
 * and collapses consecutive same-line instructions with a '|' marker for
 * cleaner output. Used by the --dump CLI flag and "disasm on" REPL command.
 */

#include "disasm.h"
#include <iostream>
#include <iomanip>

namespace {

size_t simpleInstruction(const std::string& name, size_t offset) {
    std::cout << name << '\n';
    return offset + 1;
}

size_t constantInstruction(const std::string& name, const Chunk& chunk, size_t offset) {
    int32_t val = chunk.readInt(offset + 1);
    std::cout << std::left << std::setw(16) << name << val << '\n';
    return offset + 5; // 1 byte opcode + 4 byte operand
}

size_t shortInstruction(const std::string& name, const Chunk& chunk, size_t offset) {
    uint16_t val = chunk.readShort(offset + 1);
    std::cout << std::left << std::setw(16) << name << val << '\n';
    return offset + 3; // 1 byte opcode + 2 byte operand
}

size_t byteInstruction(const std::string& name, const Chunk& chunk, size_t offset) {
    uint8_t val = chunk.code[offset + 1];
    std::cout << std::left << std::setw(16) << name << static_cast<int>(val) << '\n';
    return offset + 2; // 1 byte opcode + 1 byte operand
}

size_t jumpInstruction(const std::string& name, int sign, const Chunk& chunk, size_t offset) {
    int32_t jump = chunk.readInt(offset + 1);
    size_t target = offset + 5 + (sign * jump); // target is relative to the byte after the operand
    std::cout << std::left << std::setw(16) << name << offset << " -> " << target << '\n';
    return offset + 5;
}

} // namespace

void disassembleChunk(const Chunk& chunk, const std::string& name) {
    std::cout << "== " << name << " ==\n";
    for (size_t offset = 0; offset < chunk.code.size();) {
        offset = disassembleInstruction(chunk, offset);
    }
}

size_t disassembleInstruction(const Chunk& chunk, size_t offset) {
    int line = chunk.getLine(offset);
    // Print '|' instead of the line number when consecutive instructions share a line
    if (offset > 0 && line == chunk.getLine(offset - 1)) {
        std::cout << "   | ";
    } else {
        std::cout << std::right << std::setfill(' ') << std::setw(4) << line << " ";
    }

    std::cout << std::right << std::setfill('0') << std::setw(4) << offset 
              << "  " << std::setfill(' ') << std::left;

    uint8_t instruction = chunk.code[offset];
    switch (static_cast<Opcode>(instruction)) {
        case Opcode::PUSH_INT: return constantInstruction("PUSH_INT", chunk, offset);
        case Opcode::PUSH_BOOL: return byteInstruction("PUSH_BOOL", chunk, offset);
        case Opcode::PUSH_0: return simpleInstruction("PUSH_0", offset);
        case Opcode::PUSH_1: return simpleInstruction("PUSH_1", offset);
        case Opcode::ADD: return simpleInstruction("ADD", offset);
        case Opcode::SUB: return simpleInstruction("SUB", offset);
        case Opcode::MUL: return simpleInstruction("MUL", offset);
        case Opcode::DIV: return simpleInstruction("DIV", offset);
        case Opcode::MOD: return simpleInstruction("MOD", offset);
        case Opcode::EQ: return simpleInstruction("EQ", offset);
        case Opcode::NEQ: return simpleInstruction("NEQ", offset);
        case Opcode::LT: return simpleInstruction("LT", offset);
        case Opcode::GT: return simpleInstruction("GT", offset);
        case Opcode::LTE: return simpleInstruction("LTE", offset);
        case Opcode::GTE: return simpleInstruction("GTE", offset);
        case Opcode::BIT_XOR: return simpleInstruction("BIT_XOR", offset);
        case Opcode::BIT_AND: return simpleInstruction("BIT_AND", offset);
        case Opcode::BIT_OR: return simpleInstruction("BIT_OR", offset);
        case Opcode::SHL: return simpleInstruction("SHL", offset);
        case Opcode::SHR: return simpleInstruction("SHR", offset);
        case Opcode::BIT_NOT: return simpleInstruction("BIT_NOT", offset);
        case Opcode::NOT: return simpleInstruction("NOT", offset);
        case Opcode::NORMALIZE: return simpleInstruction("NORMALIZE", offset);
        case Opcode::NEG: return simpleInstruction("NEG", offset);
        case Opcode::POP: return simpleInstruction("POP", offset);
        case Opcode::SET_VAR: return shortInstruction("SET_VAR", chunk, offset);
        case Opcode::GET_VAR: return shortInstruction("GET_VAR", chunk, offset);
        case Opcode::SET_VAR_PUSH: return shortInstruction("SET_VAR_PUSH", chunk, offset);
        case Opcode::JMP: return jumpInstruction("JMP", 1, chunk, offset);
        case Opcode::JMP_IF_FALSE: return jumpInstruction("JMP_IF_FALSE", 1, chunk, offset);
        case Opcode::PRINT: return simpleInstruction("PRINT", offset);
        case Opcode::INPUT: return simpleInstruction("INPUT", offset);
        case Opcode::HALT: return simpleInstruction("HALT", offset);
        default:
            std::cout << "Unknown opcode " << static_cast<int>(instruction) << '\n';
            return offset + 1;
    }
}
