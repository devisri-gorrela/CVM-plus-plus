/**
 * @file compiler.cpp
 * @brief Single-pass AST-to-bytecode compiler implementation.
 *
 * Walks the AST via the Visitor pattern and emits dense bytecode into a Chunk.
 * Implements compile-time constant folding, short-circuit logical evaluation,
 * block-scoped variable management with slot reuse, and forward/backward jump
 * patching for control flow structures including for-loops with break/continue.
 */

#include "compiler.h"
#include "vm.h"
#include "error.h"
#include <optional>

static bool isPure(Expression* expr) {
    if (!expr) return true;
    switch (expr->nodeType) {
        case NodeType::AssignExpr:
        case NodeType::UpdateExpr:
        case NodeType::InputExpr:
            return false;
        case NodeType::BinaryExpr: {
            auto* b = static_cast<BinaryExpr*>(expr);
            return isPure(b->left.get()) && isPure(b->right.get());
        }
        case NodeType::UnaryExpr: {
            auto* u = static_cast<UnaryExpr*>(expr);
            return isPure(u->right.get());
        }
        case NodeType::LogicalExpr: {
            auto* l = static_cast<LogicalExpr*>(expr);
            return isPure(l->left.get()) && isPure(l->right.get());
        }
        default:
            return true;
    }
}

Compiler::Compiler() {}

Chunk Compiler::compile(const std::vector<std::unique_ptr<Statement>>& statements) {
    chunk = Chunk();
    chunk.reserve(1024);
   // Note: Compiler state (locals, variablesCount) must not be cleared here.
// The REPL requires this state to persist across sequential `compile()` executions.
// Be aware that any bugs in scope management (beginScope/endScope) will accumulate 
// state corruption across REPL lines. Use `reset()` to recover from critical errors.
    size_t initialLocals = locals.size();
    int initialDepth = currentDepth;

    try {
        for (const auto& stmt : statements) {
            stmt->accept(*this);
        }
        chunk.write(static_cast<uint8_t>(Opcode::HALT), currentLine);
        return chunk;
    } catch (...) {
        currentDepth = initialDepth;
        while (locals.size() > initialLocals) {
            freeIds.push_back(locals.back().id);
            auto& stack = localsIndex[locals.back().name];
            stack.pop_back();
            if (stack.empty()) {
                localsIndex.erase(locals.back().name);
            }
            locals.pop_back();
        }
        throw;
    }
}

int32_t Compiler::resolveVariable(const std::string& name, bool declare) {
    if (declare) {
        auto it = localsIndex.find(name);
        if (it != localsIndex.end() && !it->second.empty()) {
            // Need to check if the last declaration was in the current scope
            for (auto locIt = locals.rbegin(); locIt != locals.rend(); ++locIt) {
                if (locIt->depth < currentDepth) break;
                if (locIt->name == name) throw CompileError("Variable '" + name + "' already declared in this scope.");
            }
        }
        
        // Needs Declaring
        int32_t id;
        if (!freeIds.empty()) {
            id = freeIds.back();
            freeIds.pop_back();
        } else {
            id = variablesCount++;
            if (id >= VM::MAX_VARIABLES) {
                throw CompileError("Compile error: Variable limit exceeded (max " + std::to_string(VM::MAX_VARIABLES) + ")");
            }
        }
        locals.push_back({name, currentDepth, id});
        localsIndex[name].push_back(id);
        return id;
    }
    auto it = localsIndex.find(name);
    if (it != localsIndex.end() && !it->second.empty()) {
        return it->second.back();
    }
    throw CompileError("Undeclared variable '" + name + "'");
}

void Compiler::beginScope() {
    currentDepth++;
}

void Compiler::endScope() {
    currentDepth--;
    while (!locals.empty() && locals.back().depth > currentDepth) {
        // Note: We free the ID here so it can be reused by a new declaration.
        // The VM's `variables` array retains the stale value at this slot, but
        // this is safe because the compiler enforces that SET_VAR is always
        // emitted before GET_VAR for any reused ID.
        freeIds.push_back(locals.back().id);
        auto& stack = localsIndex[locals.back().name];
        stack.pop_back();
        if (stack.empty()) {
            localsIndex.erase(locals.back().name);
        }
        locals.pop_back();
    }
}

int Compiler::emitJmp(Opcode instruction) {
    chunk.write(static_cast<uint8_t>(instruction), currentLine);
    chunk.writeInt(0, currentLine); 
    return static_cast<int>(chunk.code.size()) - 4;
}

void Compiler::patchJmp(int offsetIndex) {
    int32_t jump = static_cast<int32_t>(chunk.code.size()) - offsetIndex - 4;
    chunk.code[offsetIndex] = (jump >> 24) & 0xFF;
    chunk.code[offsetIndex + 1] = (jump >> 16) & 0xFF;
    chunk.code[offsetIndex + 2] = (jump >> 8) & 0xFF;
    chunk.code[offsetIndex + 3] = jump & 0xFF;
}

void Compiler::emitLoop(int loopStart) {
    chunk.write(static_cast<uint8_t>(Opcode::JMP), currentLine);
    // After the VM reads the 4-byte operand, ip will be at (chunk.code.size() + 4).
    // We need to jump back to loopStart, so offset = loopStart - (here + 4).
    int32_t offset = static_cast<int32_t>(chunk.code.size()) - loopStart + 4;
    chunk.writeInt(-offset, currentLine);
}

void Compiler::visitBinaryExpr(BinaryExpr& expr) {
    currentLine = expr.line;

    // Attempt constant folding for two literal operands
    if (expr.left->nodeType == NodeType::LiteralExpr && expr.right->nodeType == NodeType::LiteralExpr) {
        auto* leftLit = static_cast<LiteralExpr*>(expr.left.get());
        auto* rightLit = static_cast<LiteralExpr*>(expr.right.get());
        if (leftLit->value.type == TokenType::NUMBER && rightLit->value.type == TokenType::NUMBER) {
            auto tryFold = [&]() -> std::optional<int64_t> {
                long long aLL = std::stoll(leftLit->value.value);
                long long bLL = std::stoll(rightLit->value.value);
                if (aLL > INT32_MAX || aLL < INT32_MIN)
                    throw CompileError("Integer literal out of range in constant expression");
                if (bLL > INT32_MAX || bLL < INT32_MIN)
                    throw CompileError("Integer literal out of range in constant expression");
                int32_t a = static_cast<int32_t>(aLL);
                int32_t b = static_cast<int32_t>(bLL);
                switch (expr.op.type) {
                    case TokenType::PLUS:  return static_cast<int64_t>(a) + static_cast<int64_t>(b);
                    case TokenType::MINUS: return static_cast<int64_t>(a) - static_cast<int64_t>(b);
                    case TokenType::STAR:  return static_cast<int64_t>(a) * static_cast<int64_t>(b);
                    case TokenType::SLASH:
                        if (b == 0) throw CompileError("Division by zero in constant expression");
                        if (a == INT32_MIN && b == -1) throw CompileError("Integer overflow in constant division");
                        return static_cast<int64_t>(a) / b;
                    case TokenType::MOD:
                        if (b == 0) throw CompileError("Modulo by zero in constant expression");
                        if (a == INT32_MIN && b == -1) throw CompileError("Integer overflow in constant modulo");
                        return static_cast<int64_t>(a) % b;
                    case TokenType::BIT_XOR: return static_cast<int64_t>(a ^ b);
                    case TokenType::BIT_AND: return static_cast<int64_t>(a & b);
                    case TokenType::BIT_OR:  return static_cast<int64_t>(a | b);
                    case TokenType::SHL:
                        if (b < 0 || b >= 32) throw CompileError("Invalid shift amount");
                        return static_cast<int64_t>(static_cast<int32_t>(static_cast<uint32_t>(a) << b));
                    case TokenType::SHR: {
                        if (b < 0 || b >= 32) throw CompileError("Invalid shift amount");
                        uint32_t ua = static_cast<uint32_t>(a);
                        uint32_t shifted = ua >> b;
                        if (a < 0 && b > 0) shifted |= ~(UINT32_MAX >> b);
                        return static_cast<int64_t>(static_cast<int32_t>(shifted));
                    }
                    case TokenType::EQ_EQ:     return (a == b) ? 1LL : 0LL;
                    case TokenType::BANG_EQ:    return (a != b) ? 1LL : 0LL;
                    case TokenType::LESS:       return (a <  b) ? 1LL : 0LL;
                    case TokenType::LESS_EQ:    return (a <= b) ? 1LL : 0LL;
                    case TokenType::GREATER:    return (a >  b) ? 1LL : 0LL;
                    case TokenType::GREATER_EQ: return (a >= b) ? 1LL : 0LL;
                    default: return std::nullopt;
                }
            };
            if (auto result = tryFold()) {
                if (*result > INT32_MAX || *result < INT32_MIN)
                    throw CompileError("Integer overflow in constant expression");
                if (*result == 0) {
                    chunk.write(static_cast<uint8_t>(Opcode::PUSH_0), currentLine);
                } else if (*result == 1) {
                    chunk.write(static_cast<uint8_t>(Opcode::PUSH_1), currentLine);
                } else {
                    chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT), currentLine);
                    chunk.writeInt(static_cast<int32_t>(*result), currentLine);
                }
                return;
            }
        }
    }

    // Runtime evaluation
    expr.left->accept(*this);
    expr.right->accept(*this);

    switch (expr.op.type) {
        case TokenType::PLUS: chunk.write(static_cast<uint8_t>(Opcode::ADD), currentLine); break;
        case TokenType::MINUS: chunk.write(static_cast<uint8_t>(Opcode::SUB), currentLine); break;
        case TokenType::STAR: chunk.write(static_cast<uint8_t>(Opcode::MUL), currentLine); break;
        case TokenType::SLASH: chunk.write(static_cast<uint8_t>(Opcode::DIV), currentLine); break;
        case TokenType::MOD: chunk.write(static_cast<uint8_t>(Opcode::MOD), currentLine); break;
        case TokenType::EQ_EQ: chunk.write(static_cast<uint8_t>(Opcode::EQ), currentLine); break;
        case TokenType::BANG_EQ: chunk.write(static_cast<uint8_t>(Opcode::NEQ), currentLine); break;
        case TokenType::LESS: chunk.write(static_cast<uint8_t>(Opcode::LT), currentLine); break;
        case TokenType::LESS_EQ: chunk.write(static_cast<uint8_t>(Opcode::LTE), currentLine); break;
        case TokenType::GREATER: chunk.write(static_cast<uint8_t>(Opcode::GT), currentLine); break;
        case TokenType::GREATER_EQ: chunk.write(static_cast<uint8_t>(Opcode::GTE), currentLine); break;
        case TokenType::BIT_XOR: chunk.write(static_cast<uint8_t>(Opcode::BIT_XOR), currentLine); break;
        case TokenType::BIT_AND: chunk.write(static_cast<uint8_t>(Opcode::BIT_AND), currentLine); break;
        case TokenType::SHL: chunk.write(static_cast<uint8_t>(Opcode::SHL), currentLine); break;
        case TokenType::SHR: chunk.write(static_cast<uint8_t>(Opcode::SHR), currentLine); break;
        case TokenType::BIT_OR: chunk.write(static_cast<uint8_t>(Opcode::BIT_OR), currentLine); break;
        default: throw CompileError("Unknown binary operator");
    }
}

void Compiler::visitLogicalExpr(LogicalExpr& expr) {
    currentLine = expr.line;

    // --- O2: Constant folding for logical expressions with literal operands ---
    if (expr.left->nodeType == NodeType::LiteralExpr && expr.right->nodeType == NodeType::LiteralExpr) {
        auto* leftLit = static_cast<LiteralExpr*>(expr.left.get());
        auto* rightLit = static_cast<LiteralExpr*>(expr.right.get());
        auto toBool = [](const LiteralExpr* lit) -> int {
            if (lit->value.type == TokenType::TRUE_LIT) return 1;
            if (lit->value.type == TokenType::FALSE_LIT) return 0;
            if (lit->value.type == TokenType::NUMBER) {
                try { return std::stoi(lit->value.value) != 0 ? 1 : 0; }
                catch (...) { return -1; }
            }
            return -1;
        };
        int lv = toBool(leftLit);
        int rv = toBool(rightLit);
        if (lv >= 0 && rv >= 0) {
            int result;
            if (expr.op.type == TokenType::AND_AND) {
                result = (lv && rv) ? 1 : 0;
            } else {
                result = (lv || rv) ? 1 : 0;
            }
            if (result == 0) {
                chunk.write(static_cast<uint8_t>(Opcode::PUSH_0), currentLine);
            } else {
                chunk.write(static_cast<uint8_t>(Opcode::PUSH_1), currentLine);
            }
            return;
        }
    }

    expr.left->accept(*this);
    
    if (expr.op.type == TokenType::AND_AND) {
        int endJump = emitJmp(Opcode::JMP_IF_FALSE);
        
        expr.right->accept(*this);
        chunk.write(static_cast<uint8_t>(Opcode::NORMALIZE), currentLine);
        int skipJump = emitJmp(Opcode::JMP);
        
        patchJmp(endJump);
        chunk.write(static_cast<uint8_t>(Opcode::PUSH_0), currentLine);
        
        patchJmp(skipJump);
    } else if (expr.op.type == TokenType::OR_OR) {
        int elseJump = emitJmp(Opcode::JMP_IF_FALSE);
        
        chunk.write(static_cast<uint8_t>(Opcode::PUSH_1), currentLine);
        int endJump = emitJmp(Opcode::JMP);
        
        patchJmp(elseJump);
        expr.right->accept(*this);
        chunk.write(static_cast<uint8_t>(Opcode::NORMALIZE), currentLine);
        
        patchJmp(endJump);
    } else {
        throw CompileError("Unknown logical operator");
    }
}

void Compiler::visitUnaryExpr(UnaryExpr& expr) {
    currentLine = expr.line;

    // --- Attempt constant folding for literal operands ---
    auto tryFoldUnary = [&]() -> bool {
        if (expr.right->nodeType != NodeType::LiteralExpr) return false;
        auto* lit = static_cast<LiteralExpr*>(expr.right.get());

        if (lit->value.type == TokenType::NUMBER) {
            long long val;
            try { val = std::stoll(lit->value.value); }
            catch (...) { return false; }

            if (expr.op.type == TokenType::MINUS) {
                long long result = -val;
                if (result > INT32_MAX || result < INT32_MIN)
                    throw CompileError("Integer overflow in constant negation");
                if (result == 0) chunk.write(static_cast<uint8_t>(Opcode::PUSH_0), currentLine);
                else if (result == 1) chunk.write(static_cast<uint8_t>(Opcode::PUSH_1), currentLine);
                else {
                    chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT), currentLine);
                    chunk.writeInt(static_cast<int32_t>(result), currentLine);
                }
                return true;
            } else if (expr.op.type == TokenType::BIT_NOT) {
                if (val > INT32_MAX || val < INT32_MIN)
                    throw CompileError("Integer literal out of range");
                int32_t result = ~static_cast<int32_t>(val);
                if (result == 0) chunk.write(static_cast<uint8_t>(Opcode::PUSH_0), currentLine);
                else if (result == 1) chunk.write(static_cast<uint8_t>(Opcode::PUSH_1), currentLine);
                else {
                    chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT), currentLine);
                    chunk.writeInt(result, currentLine);
                }
                return true;
            } else if (expr.op.type == TokenType::NOT) {
                if (val > INT32_MAX || val < INT32_MIN)
                    throw CompileError("Integer literal out of range");
                chunk.write(static_cast<uint8_t>(val == 0 ? Opcode::PUSH_1 : Opcode::PUSH_0), currentLine);
                return true;
            } else if (expr.op.type == TokenType::PLUS) {
                if (val > INT32_MAX || val < INT32_MIN)
                    throw CompileError("Integer literal out of range");
                if (val == 0) chunk.write(static_cast<uint8_t>(Opcode::PUSH_0), currentLine);
                else if (val == 1) chunk.write(static_cast<uint8_t>(Opcode::PUSH_1), currentLine);
                else {
                    chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT), currentLine);
                    chunk.writeInt(static_cast<int32_t>(val), currentLine);
                }
                return true;
            }
        }

        if (lit->value.type == TokenType::TRUE_LIT || lit->value.type == TokenType::FALSE_LIT) {
            int32_t boolVal = (lit->value.type == TokenType::TRUE_LIT) ? 1 : 0;
            if (expr.op.type == TokenType::NOT) {
                chunk.write(static_cast<uint8_t>(boolVal == 0 ? Opcode::PUSH_1 : Opcode::PUSH_0), currentLine);
                return true;
            }
        }
        return false;
    };

    if (tryFoldUnary()) return;

    // --- Runtime evaluation ---
    expr.right->accept(*this);
    if (expr.op.type == TokenType::BIT_NOT) {
        chunk.write(static_cast<uint8_t>(Opcode::BIT_NOT), currentLine);
    } else if (expr.op.type == TokenType::NOT) {
        chunk.write(static_cast<uint8_t>(Opcode::NOT), currentLine);
    } else if (expr.op.type == TokenType::MINUS) {
        chunk.write(static_cast<uint8_t>(Opcode::NEG), currentLine);
    } else if (expr.op.type == TokenType::PLUS) {
        // Unary plus is a no-op -- value is already on the stack
    } else {
        throw CompileError("Unknown unary operator");
    }
}

void Compiler::visitLiteralExpr(LiteralExpr& expr) {
    currentLine = expr.line;

    if (expr.value.type == TokenType::NUMBER) {
        try {
            long long val = std::stoll(expr.value.value);
            if (val > INT32_MAX || val < INT32_MIN) {
                throw CompileError("Integer literal out of range: " + expr.value.value);
            }
            if (val == 0) chunk.write(static_cast<uint8_t>(Opcode::PUSH_0), currentLine);
            else if (val == 1) chunk.write(static_cast<uint8_t>(Opcode::PUSH_1), currentLine);
            else {
                chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT), currentLine);
                chunk.writeInt(static_cast<int32_t>(val), currentLine);
            }
        } catch (const std::out_of_range&) {
            throw CompileError("Integer literal out of range: " + expr.value.value);
        } catch (const std::invalid_argument&) {
            throw CompileError("Invalid integer literal: " + expr.value.value);
        }
    } else if (expr.value.type == TokenType::TRUE_LIT) {
        chunk.write(static_cast<uint8_t>(Opcode::PUSH_1), currentLine);
    } else if (expr.value.type == TokenType::FALSE_LIT) {
        chunk.write(static_cast<uint8_t>(Opcode::PUSH_0), currentLine);
    } else {
        throw CompileError("Unknown literal type");
    }
}

void Compiler::visitVariableExpr(VariableExpr& expr) {
    currentLine = expr.line;

    int32_t id = resolveVariable(expr.name.value, false);
    chunk.write(static_cast<uint8_t>(Opcode::GET_VAR), currentLine);
    chunk.writeShort(static_cast<uint16_t>(id), currentLine);
}

void Compiler::visitInputExpr(InputExpr&) {
    chunk.write(static_cast<uint8_t>(Opcode::INPUT), currentLine);
}

void Compiler::visitAssignExpr(AssignExpr& expr) {
    currentLine = expr.line;

    expr.value->accept(*this);
    int32_t id = resolveVariable(expr.name.value, false);
    chunk.write(static_cast<uint8_t>(Opcode::SET_VAR_PUSH), currentLine);
    chunk.writeShort(static_cast<uint16_t>(id), currentLine);
}

void Compiler::visitUpdateExpr(UpdateExpr& expr) {
    currentLine = expr.line;

    int32_t id = resolveVariable(expr.name.value, false);
    if (expr.isPrefix) {
        // Prefix (++x / --x): update variable, leave NEW value on stack.
        chunk.write(static_cast<uint8_t>(Opcode::GET_VAR), currentLine);
        chunk.writeShort(static_cast<uint16_t>(id), currentLine);
        chunk.write(static_cast<uint8_t>(Opcode::PUSH_1), currentLine);
        chunk.write(static_cast<uint8_t>(expr.isIncrement ? Opcode::ADD : Opcode::SUB), currentLine);
        chunk.write(static_cast<uint8_t>(Opcode::SET_VAR_PUSH), currentLine);
        chunk.writeShort(static_cast<uint16_t>(id), currentLine);
    } else {
        // Postfix (x++ / x--): leave OLD value on stack, then update variable.
        chunk.write(static_cast<uint8_t>(Opcode::GET_VAR), currentLine);
        chunk.writeShort(static_cast<uint16_t>(id), currentLine); // old value (result)
        chunk.write(static_cast<uint8_t>(Opcode::GET_VAR), currentLine);
        chunk.writeShort(static_cast<uint16_t>(id), currentLine); // old value (for computation)
        chunk.write(static_cast<uint8_t>(Opcode::PUSH_1), currentLine);
        chunk.write(static_cast<uint8_t>(expr.isIncrement ? Opcode::ADD : Opcode::SUB), currentLine);
        chunk.write(static_cast<uint8_t>(Opcode::SET_VAR), currentLine);
        chunk.writeShort(static_cast<uint16_t>(id), currentLine);
    }
}

void Compiler::visitBlockStmt(BlockStmt& stmt) {
    currentLine = stmt.line;

    beginScope();
    for (const auto& s : stmt.statements) {
        s->accept(*this);
    }
    endScope();
}

void Compiler::visitExpressionStmt(ExpressionStmt& stmt) {
    currentLine = stmt.line;

    if (isPure(stmt.expr.get())) return;
    stmt.expr->accept(*this);
    chunk.write(static_cast<uint8_t>(Opcode::POP), currentLine); 
}

void Compiler::visitLetStmt(LetStmt& stmt) {
    currentLine = stmt.line;

    if (stmt.initializer) {
        stmt.initializer->accept(*this);
    } else {
        chunk.write(static_cast<uint8_t>(Opcode::PUSH_0), currentLine);
    }
    int32_t id = resolveVariable(stmt.name.value, true);
    chunk.write(static_cast<uint8_t>(Opcode::SET_VAR), currentLine);
    chunk.writeShort(static_cast<uint16_t>(id), currentLine);
}

void Compiler::visitIfStmt(IfStmt& stmt) {
    currentLine = stmt.line;

    bool isStaticTrue = false;
    bool isStaticFalse = false;
    if (stmt.condition->nodeType == NodeType::LiteralExpr) {
        auto* lit = static_cast<LiteralExpr*>(stmt.condition.get());
        if (lit->value.type == TokenType::TRUE_LIT) isStaticTrue = true;
        else if (lit->value.type == TokenType::FALSE_LIT) isStaticFalse = true;
        else if (lit->value.type == TokenType::NUMBER) {
            try { if (std::stoll(lit->value.value) == 0) isStaticFalse = true; else isStaticTrue = true; } catch(...) {}
        }
    }

    if (isStaticTrue) {
        stmt.thenBranch->accept(*this);
        return;
    }
    if (isStaticFalse) {
        if (stmt.elseBranch) stmt.elseBranch->accept(*this);
        return;
    }

    stmt.condition->accept(*this);
    int thenJmp = emitJmp(Opcode::JMP_IF_FALSE);
    
    stmt.thenBranch->accept(*this);
    
    if (stmt.elseBranch) {
        int elseJmp = emitJmp(Opcode::JMP);
        patchJmp(thenJmp);
        stmt.elseBranch->accept(*this);
        patchJmp(elseJmp);
    } else {
        patchJmp(thenJmp);
    }
}

void Compiler::visitWhileStmt(WhileStmt& stmt) {
    currentLine = stmt.line;

    bool isStaticFalse = false;
    if (stmt.condition->nodeType == NodeType::LiteralExpr) {
        auto* lit = static_cast<LiteralExpr*>(stmt.condition.get());
        if (lit->value.type == TokenType::FALSE_LIT) isStaticFalse = true;
        else if (lit->value.type == TokenType::NUMBER) {
            try { if (std::stoll(lit->value.value) == 0) isStaticFalse = true; } catch(...) {}
        }
    }

    if (isStaticFalse) return;

    LoopContext ctx;
    ctx.loopStart = static_cast<int>(chunk.code.size());
    ctx.continuePatchesToIncrement = false;
    loopStack.push_back(ctx);

    int loopStart = static_cast<int>(chunk.code.size());
    stmt.condition->accept(*this);
    
    int exitJmp = emitJmp(Opcode::JMP_IF_FALSE);
    stmt.body->accept(*this);
    
    emitLoop(loopStart);
    patchJmp(exitJmp);

    // Patch any `break;` jumps to loop end.
    for (int br : loopStack.back().breakJumps) {
        patchJmp(br);
    }
    // `continue;` in while loops is emitted as a backward jump immediately.
    loopStack.pop_back();
}

void Compiler::visitForStmt(ForStmt& stmt) {
    currentLine = stmt.line;

    // Keep `let` variables inside the for-loop initializer scoped to the loop.
    beginScope();

    LoopContext ctx;
    ctx.continuePatchesToIncrement = true; // continue jumps forward to increment start
    loopStack.push_back(ctx);

    if (stmt.initializer) {
        stmt.initializer->accept(*this);
    }

    int loopStart = static_cast<int>(chunk.code.size());
    loopStack.back().loopStart = loopStart;
    
    int exitJmp = -1;
    if (stmt.condition) {
        stmt.condition->accept(*this);
        exitJmp = emitJmp(Opcode::JMP_IF_FALSE);
    }

    stmt.body->accept(*this);

    // `continue;` inside the body should jump to the increment section (forward).
    for (int cont : loopStack.back().continueJumps) {
        patchJmp(cont);
    }

    if (stmt.increment) {
        stmt.increment->accept(*this);
        // INVARIANT: All valid increment expressions (assignments, ++/-- updates,
        // binary ops, literals) push exactly one value onto the stack. This POP
        // discards that unused result. If new expression types are added that
        // don't push a value, this must be revisited.
        chunk.write(static_cast<uint8_t>(Opcode::POP), currentLine);
    }

    emitLoop(loopStart);
    if (exitJmp != -1) {
        patchJmp(exitJmp);
    }

    // Patch any `break;` jumps to loop end (after condition fails / after loop).
    for (int br : loopStack.back().breakJumps) {
        patchJmp(br);
    }
    loopStack.pop_back();

    endScope();
}

void Compiler::visitBreakStmt(BreakStmt&) {
    if (loopStack.empty()) {
        throw CompileError("Cannot use 'break' outside of a loop");
    }
    int j = emitJmp(Opcode::JMP);
    loopStack.back().breakJumps.push_back(j);
}

void Compiler::visitContinueStmt(ContinueStmt&) {
    if (loopStack.empty()) {
        throw CompileError("Cannot use 'continue' outside of a loop");
    }
    LoopContext& ctx = loopStack.back();
    if (ctx.continuePatchesToIncrement) {
        int j = emitJmp(Opcode::JMP);
        ctx.continueJumps.push_back(j);
    } else {
        emitLoop(ctx.loopStart);
    }
}

void Compiler::visitPrintStmt(PrintStmt& stmt) {
    currentLine = stmt.line;

    stmt.expr->accept(*this);
    chunk.write(static_cast<uint8_t>(Opcode::PRINT), currentLine);
}
