/**
 * @file vm.cpp
 * @brief Stack-based virtual machine bytecode interpreter implementation.
 *
 * Executes compiled bytecode from a Chunk using a fetch-decode-execute loop.
 * Implements all 27 opcodes with overflow detection (int64_t intermediates),
 * portable arithmetic right shifts, and stack pointer save/restore for REPL
 * error recovery. Reports runtime errors with source-line mapping via the
 * Chunk's RLE-compressed line table.
 */

#include "vm.h"
#include "error.h"
#include <iostream>
#include <limits>

VM::VM() : sp(0) {
    variables.resize(MAX_VARIABLES, 0);
}

void VM::push(int32_t value) {
    if (sp >= STACK_MAX) throw RuntimeError("Stack overflow");
    stack[sp++] = value;
}

int32_t VM::pop() {
    if (sp == 0) throw RuntimeError("Stack underflow");
    return stack[--sp];
}

int32_t VM::peekStack() const {
    if (sp == 0) return 0;
    return stack[sp - 1];
}

void VM::execute(const Chunk& chunk) {
    size_t savedSp = sp; // saved so we can restore the stack on error
    size_t ip = 0;
    const uint8_t* code = chunk.code.data();
    size_t codeSize = chunk.code.size();

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    static void* dispatch_table[] = {
        &&do_PUSH_INT, &&do_PUSH_BOOL, &&do_PUSH_0, &&do_PUSH_1,
        &&do_ADD, &&do_SUB, &&do_MUL, &&do_DIV, &&do_MOD,
        &&do_EQ, &&do_NEQ, &&do_LT, &&do_GT, &&do_LTE, &&do_GTE,
        &&do_BIT_XOR, &&do_BIT_AND, &&do_BIT_OR, &&do_SHL, &&do_SHR, &&do_BIT_NOT,
        &&do_NOT, &&do_NORMALIZE, &&do_NEG, &&do_POP,
        &&do_SET_VAR, &&do_GET_VAR, &&do_SET_VAR_PUSH,
        &&do_JMP, &&do_JMP_IF_FALSE, &&do_PRINT, &&do_INPUT, &&do_HALT
    };

    #define DISPATCH() do { \
        if (ip >= codeSize) throw RuntimeError("Execution reached end of chunk without HALT"); \
        goto *dispatch_table[code[ip++]]; \
    } while(0)
    #define TARGET(op) do_##op:
#else
    #define DISPATCH() goto next_instr
    #define TARGET(op) case Opcode::op:
#endif

    try {
#if defined(__GNUC__) || defined(__clang__)
        DISPATCH();
#else
    next_instr:
        if (ip >= codeSize) throw RuntimeError("Execution reached end of chunk without HALT");
        switch (static_cast<Opcode>(code[ip++]))
#endif
        {
            TARGET(PUSH_INT) {
                int32_t val = chunk.readInt(ip);
                ip += 4;
                push(val);
                DISPATCH();
            }
            TARGET(PUSH_BOOL) {
                int32_t val = code[ip++];
                push(val);
                DISPATCH();
            }
            TARGET(PUSH_0) {
                push(0);
                DISPATCH();
            }
            TARGET(PUSH_1) {
                push(1);
                DISPATCH();
            }
            TARGET(ADD) {
                int32_t b = pop();
                int32_t a = pop();
                int64_t result = static_cast<int64_t>(a) + static_cast<int64_t>(b);
                if (result > INT32_MAX || result < INT32_MIN) throw RuntimeError("Integer overflow in addition");
                push(static_cast<int32_t>(result));
                DISPATCH();
            }
            TARGET(SUB) {
                int32_t b = pop();
                int32_t a = pop();
                int64_t result = static_cast<int64_t>(a) - static_cast<int64_t>(b);
                if (result > INT32_MAX || result < INT32_MIN) throw RuntimeError("Integer overflow in subtraction");
                push(static_cast<int32_t>(result));
                DISPATCH();
            }
            TARGET(MUL) {
                int32_t b = pop();
                int32_t a = pop();
                int64_t result = static_cast<int64_t>(a) * static_cast<int64_t>(b);
                if (result > INT32_MAX || result < INT32_MIN) throw RuntimeError("Integer overflow in multiplication");
                push(static_cast<int32_t>(result));
                DISPATCH();
            }
            TARGET(DIV) {
                int32_t b = pop();
                int32_t a = pop();
                if (b == 0) throw RuntimeError("Division by zero");
                if (a == INT32_MIN && b == -1) throw RuntimeError("Integer overflow in division"); 
                push(a / b);
                DISPATCH();
            }
            TARGET(MOD) {
                int32_t b = pop();
                int32_t a = pop();
                if (b == 0) throw RuntimeError("Modulo by zero");
                if (a == INT32_MIN && b == -1) throw RuntimeError("Integer overflow in modulo");
                push(a % b);
                DISPATCH();
            }
            TARGET(EQ) {
                int32_t b = pop();
                int32_t a = pop();
                push(a == b ? 1 : 0);
                DISPATCH();
            }
            TARGET(NEQ) {
                int32_t b = pop();
                int32_t a = pop();
                push(a != b ? 1 : 0);
                DISPATCH();
            }
            TARGET(LT) {
                int32_t b = pop();
                int32_t a = pop();
                push(a < b ? 1 : 0);
                DISPATCH();
            }
            TARGET(LTE) {
                int32_t b = pop();
                int32_t a = pop();
                push(a <= b ? 1 : 0);
                DISPATCH();
            }
            TARGET(GT) {
                int32_t b = pop();
                int32_t a = pop();
                push(a > b ? 1 : 0);
                DISPATCH();
            }
            TARGET(GTE) {
                int32_t b = pop();
                int32_t a = pop();
                push(a >= b ? 1 : 0);
                DISPATCH();
            }
            TARGET(BIT_XOR) {
                int32_t b = pop();
                int32_t a = pop();
                push(a ^ b);
                DISPATCH();
            }
            TARGET(BIT_AND) {
                int32_t b = pop();
                int32_t a = pop();
                push(a & b);
                DISPATCH();
            }
            TARGET(BIT_OR) {
                int32_t b = pop();
                int32_t a = pop();
                push(a | b);
                DISPATCH();
            }
            TARGET(SHL) {
                int32_t b = pop();
                int32_t a = pop();
                if (b < 0 || b >= 32) throw RuntimeError("Invalid shift amount");
                push(static_cast<int32_t>(static_cast<uint32_t>(a) << b));
                DISPATCH();
            }
            TARGET(SHR) {
                int32_t b = pop();
                int32_t a = pop();
                if (b < 0 || b >= 32) throw RuntimeError("Invalid shift amount");
                uint32_t ua = static_cast<uint32_t>(a);
                uint32_t shifted = ua >> b;
                if (a < 0 && b > 0) {
                    uint32_t mask = ~(UINT32_MAX >> b);
                    shifted |= mask;
                }
                push(static_cast<int32_t>(shifted));
                DISPATCH();
            }
            TARGET(BIT_NOT) {
                int32_t a = pop();
                push(~a);
                DISPATCH();
            }
            TARGET(NOT) {
                int32_t a = pop();
                push(a == 0 ? 1 : 0);
                DISPATCH();
            }
            TARGET(NORMALIZE) {
                int32_t a = pop();
                push(a == 0 ? 0 : 1);
                DISPATCH();
            }
            TARGET(NEG) {
                int32_t a = pop();
                if (a == INT32_MIN) throw RuntimeError("Integer overflow in negation");
                push(-a);
                DISPATCH();
            }
            TARGET(POP) {
                pop();
                DISPATCH();
            }
            TARGET(SET_VAR) {
                uint16_t id = chunk.readShort(ip);
                ip += 2;
                variables[id] = pop();
                DISPATCH();
            }
            TARGET(SET_VAR_PUSH) {
                uint16_t id = chunk.readShort(ip);
                ip += 2;
                int32_t val = pop();
                variables[id] = val;
                push(val);
                DISPATCH();
            }
            TARGET(GET_VAR) {
                uint16_t id = chunk.readShort(ip);
                ip += 2;
                push(variables[id]);
                DISPATCH();
            }
            TARGET(JMP) {
                int32_t offset = chunk.readInt(ip);
                ip += 4;
                size_t newIp = static_cast<size_t>(static_cast<int64_t>(ip) + offset);
                if (newIp > codeSize) throw RuntimeError("Jump target out of bounds");
                ip = newIp;
                DISPATCH();
            }
            TARGET(JMP_IF_FALSE) {
                int32_t offset = chunk.readInt(ip);
                ip += 4;
                int32_t cond = pop();
                if (cond == 0) {
                    size_t newIp = static_cast<size_t>(static_cast<int64_t>(ip) + offset);
                    if (newIp > codeSize) throw RuntimeError("Jump target out of bounds");
                    ip = newIp;
                }
                DISPATCH();
            }
            TARGET(PRINT) {
                int32_t val = pop();
                std::cout << val << '\n';
                DISPATCH();
            }
            TARGET(INPUT) {
                int32_t val;
                if (!(std::cin >> val)) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    throw RuntimeError("Invalid input: expected an integer");
                }
                push(val);
                DISPATCH();
            }
            TARGET(HALT) {
                return;
            }
#if !defined(__GNUC__) && !defined(__clang__)
            default:
                throw RuntimeError("Unknown Opcode");
#endif
        }
    throw RuntimeError("Execution reached end of chunk without HALT");
    } catch (const RuntimeError& e) {
        sp = savedSp;
        int line = (ip > 0) ? chunk.getLine(ip - 1) : -1;
        if (line > 0 && e.line == 0) {
            throw RuntimeError(e.getRawMsg(), line);
        }
        throw;
    } catch (...) {
        sp = savedSp;
        throw;
    }
}
