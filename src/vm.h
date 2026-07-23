/**
 * @file vm.h
 * @brief Stack-based virtual machine execution engine for CVM++ bytecode.
 *
 * The VM interprets bytecode produced by the Compiler using a fixed-size
 * operand stack (2048 slots) with overflow/underflow protection. Implements
 * portable arithmetic right shifts, integer overflow detection using int64_t
 * intermediates, and source-line mapping via the Chunk's RLE table for
 * accurate runtime error reporting. Supports error recovery for REPL usage.
 */

#pragma once
#include "opcode.h"
#include "chunk.h"
#include <array>
#include <vector>

class VM {
public:
    static constexpr int32_t MAX_VARIABLES = 65536;

    VM();
    void execute(const Chunk& chunk);
    void reset() { sp = 0; std::fill(variables.begin(), variables.end(), 0); }

    // For testing/debugging, we can inspect stack or variables
    [[nodiscard]] int32_t peekStack() const;

private:
    static constexpr size_t STACK_MAX = 2048;
    std::array<int32_t, STACK_MAX> stack;
    size_t sp; // Stack pointer

    std::vector<int32_t> variables;  // flat-addressable variable slots

    void push(int32_t value);
    int32_t pop();
};
