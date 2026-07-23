/**
 * @file disasm.h
 * @brief Bytecode disassembler for human-readable CVM++ instruction dumps.
 *
 * Provides disassembleChunk() and disassembleInstruction() functions that
 * pretty-print compiled bytecode with source line annotations and resolved
 * jump targets. Supports four instruction formats: simple (1 byte), constant
 * (1 + 4 bytes), byte (1 + 1 byte), and jump (1 + 4 bytes). Accessible
 * via the --dump CLI flag or the "disasm on" REPL command.
 */

#pragma once
#include "opcode.h"
#include "chunk.h"
#include <string>

void disassembleChunk(const Chunk& chunk, const std::string& name);
size_t disassembleInstruction(const Chunk& chunk, size_t offset);
