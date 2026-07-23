/**
 * @file opcode.h
 * @brief Bytecode instruction set definition for the CVM++ virtual machine.
 *
 * Defines the complete Opcode enum as uint8_t values used by the compiler
 * to emit bytecode and by the VM to execute programs. The instruction set
 * covers arithmetic, comparison, bitwise, logical, variable access, control
 * flow, and I/O operations. All opcodes are designed for compact single-byte
 * encoding with inline operands for maximum bytecode density.
 */

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>

enum class Opcode : uint8_t {
    PUSH_INT,
    PUSH_BOOL,
    PUSH_0,
    PUSH_1,
    ADD, SUB, MUL, DIV, MOD,
    EQ, NEQ, LT, GT, LTE, GTE,
    BIT_XOR, BIT_AND, BIT_OR, SHL, SHR, BIT_NOT,
    NOT, NORMALIZE, NEG, POP,
    SET_VAR,
    GET_VAR,
    SET_VAR_PUSH,
    JMP,
    JMP_IF_FALSE,
    PRINT,
    INPUT,
    HALT
};


