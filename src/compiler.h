/**
 * @file compiler.h
 * @brief Single-pass AST-to-bytecode compiler with constant folding.
 *
 * The Compiler implements the ASTVisitor interface to walk the AST and emit
 * dense bytecode directly into a Chunk. Features include compile-time constant
 * folding for binary, unary, and logical expressions, short-circuit evaluation,
 * deep block scoping with variable shadowing and slot reuse, and a LoopContext
 * stack for managing break/continue jumps in nested loops. REPL-aware design
 * allows variable bindings to persist across compilation calls.
 */

#pragma once
#include "ast.h"
#include "opcode.h"
#include "chunk.h"
#include <string>
#include <vector>
#include <unordered_map>

struct Local {
    std::string name;
    int depth;
    int32_t id;
};

class Compiler : public ASTVisitor {
public:
    Compiler();
    Chunk compile(const std::vector<std::unique_ptr<Statement>>& statements);
    void reset() { chunk = Chunk(); locals.clear(); localsIndex.clear(); freeIds.clear(); variablesCount = 0; currentDepth = 0; loopStack.clear(); currentLine = 0; }

    void visitBinaryExpr(BinaryExpr& expr) override;
    void visitUnaryExpr(UnaryExpr& expr) override;
    void visitLiteralExpr(LiteralExpr& expr) override;
    void visitLogicalExpr(LogicalExpr& expr) override;
    void visitVariableExpr(VariableExpr& expr) override;
    void visitInputExpr(InputExpr& expr) override;
    void visitAssignExpr(AssignExpr& expr) override;
    void visitUpdateExpr(UpdateExpr& expr) override;

    void visitBlockStmt(BlockStmt& stmt) override;
    void visitExpressionStmt(ExpressionStmt& stmt) override;
    void visitLetStmt(LetStmt& stmt) override;
    void visitIfStmt(IfStmt& stmt) override;
    void visitWhileStmt(WhileStmt& stmt) override;
    void visitForStmt(ForStmt& stmt) override;
    void visitBreakStmt(BreakStmt& stmt) override;
    void visitContinueStmt(ContinueStmt& stmt) override;
    void visitPrintStmt(PrintStmt& stmt) override;

private:
    Chunk chunk;
    std::vector<Local> locals;
    std::unordered_map<std::string, std::vector<int32_t>> localsIndex;
    int variablesCount = 0;
    std::vector<int32_t> freeIds;
    int currentDepth = 0;
    int currentLine = 0;
    
    int32_t resolveVariable(const std::string& name, bool declare);
    void beginScope();
    void endScope();
    
    int emitJmp(Opcode instruction);
    void patchJmp(int offsetIndex);
    void emitLoop(int loopStart);

    struct LoopContext {
        int loopStart = 0;                 // for backward continue (while)
        bool continuePatchesToIncrement = false; // for forward continue (for)
        std::vector<int> breakJumps;       // operand indices to patch to loop end
        std::vector<int> continueJumps;    // operand indices to patch to increment start
    };
    std::vector<LoopContext> loopStack;
};
