/**
 * @file ast.h
 * @brief Abstract Syntax Tree node definitions and Visitor pattern interface.
 *
 * Defines the complete AST node hierarchy using a compile-time NodeType enum
 * instead of RTTI (dynamic_cast) for efficient type dispatch. Covers all
 * expression types (binary, unary, logical, literal, variable, input, assign,
 * update) and statement types (block, expression, let, if, while, for, break,
 * continue, print). Implements the Visitor pattern for clean separation between
 * the AST structure and compilation logic.
 */

#pragma once
#include "token.h"
#include <memory>
#include <vector>

class ASTVisitor;

enum class NodeType {
    // Expressions
    BinaryExpr, UnaryExpr, LogicalExpr, LiteralExpr,
    VariableExpr, InputExpr, AssignExpr, UpdateExpr,
    // Statements
    BlockStmt, ExpressionStmt, LetStmt, IfStmt,
    WhileStmt, ForStmt, BreakStmt, ContinueStmt, PrintStmt
};

struct ASTNode {
    const NodeType nodeType;
    int line;
    explicit ASTNode(NodeType type, int line_) : nodeType(type), line(line_) {}
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
};

struct Expression : public ASTNode {
    using ASTNode::ASTNode;
};
struct Statement : public ASTNode {
    using ASTNode::ASTNode;
};

struct BinaryExpr : public Expression {
    std::unique_ptr<Expression> left;
    Token op;
    std::unique_ptr<Expression> right;
    
    BinaryExpr(std::unique_ptr<Expression> left_, Token op_, std::unique_ptr<Expression> right_)
        : Expression(NodeType::BinaryExpr, op_.line), left(std::move(left_)), op(std::move(op_)), right(std::move(right_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct UnaryExpr : public Expression {
    Token op;
    std::unique_ptr<Expression> right;

    UnaryExpr(Token op_, std::unique_ptr<Expression> right_)
        : Expression(NodeType::UnaryExpr, op_.line), op(std::move(op_)), right(std::move(right_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct LogicalExpr : public Expression {
    std::unique_ptr<Expression> left;
    Token op;
    std::unique_ptr<Expression> right;

    LogicalExpr(std::unique_ptr<Expression> left_, Token op_, std::unique_ptr<Expression> right_)
        : Expression(NodeType::LogicalExpr, op_.line), left(std::move(left_)), op(std::move(op_)), right(std::move(right_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct LiteralExpr : public Expression {
    Token value;

    explicit LiteralExpr(Token value_) : Expression(NodeType::LiteralExpr, value_.line), value(std::move(value_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct VariableExpr : public Expression {
    Token name;

    explicit VariableExpr(Token name_) : Expression(NodeType::VariableExpr, name_.line), name(std::move(name_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct InputExpr : public Expression {
    explicit InputExpr(int line_) : Expression(NodeType::InputExpr, line_) {}
    void accept(ASTVisitor& visitor) override;
};

struct AssignExpr : public Expression {
    Token name;
    std::unique_ptr<Expression> value;

    AssignExpr(Token name_, std::unique_ptr<Expression> value_)
        : Expression(NodeType::AssignExpr, name_.line), name(std::move(name_)), value(std::move(value_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct UpdateExpr : public Expression {
    Token name;
    bool isIncrement;
    bool isPrefix;

    UpdateExpr(Token name_, bool isIncrement_, bool isPrefix_)
        : Expression(NodeType::UpdateExpr, name_.line), name(std::move(name_)), isIncrement(isIncrement_), isPrefix(isPrefix_) {}
    void accept(ASTVisitor& visitor) override;
};

struct BlockStmt : public Statement {
    std::vector<std::unique_ptr<Statement>> statements;

    explicit BlockStmt(std::vector<std::unique_ptr<Statement>> statements_, int line_)
        : Statement(NodeType::BlockStmt, line_), statements(std::move(statements_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct ExpressionStmt : public Statement {
    std::unique_ptr<Expression> expr;

    explicit ExpressionStmt(std::unique_ptr<Expression> expr_, int line_) : Statement(NodeType::ExpressionStmt, line_), expr(std::move(expr_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct LetStmt : public Statement {
    Token name;
    std::unique_ptr<Expression> initializer;

    LetStmt(Token name_, std::unique_ptr<Expression> initializer_)
        : Statement(NodeType::LetStmt, name_.line), name(std::move(name_)), initializer(std::move(initializer_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct IfStmt : public Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch;

    IfStmt(std::unique_ptr<Expression> condition_, std::unique_ptr<Statement> thenBranch_, std::unique_ptr<Statement> elseBranch_, int line_)
        : Statement(NodeType::IfStmt, line_), condition(std::move(condition_)), thenBranch(std::move(thenBranch_)), elseBranch(std::move(elseBranch_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct WhileStmt : public Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;

    WhileStmt(std::unique_ptr<Expression> condition_, std::unique_ptr<Statement> body_, int line_)
        : Statement(NodeType::WhileStmt, line_), condition(std::move(condition_)), body(std::move(body_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct ForStmt : public Statement {
    // C-like: for (initializer ; condition ; increment) body
    std::unique_ptr<Statement> initializer;
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> increment;
    std::unique_ptr<Statement> body;

    ForStmt(std::unique_ptr<Statement> initializer_,
            std::unique_ptr<Expression> condition_,
            std::unique_ptr<Expression> increment_,
            std::unique_ptr<Statement> body_, int line_)
        : Statement(NodeType::ForStmt, line_),
          initializer(std::move(initializer_)),
          condition(std::move(condition_)),
          increment(std::move(increment_)),
          body(std::move(body_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct BreakStmt : public Statement {
    explicit BreakStmt(int line_) : Statement(NodeType::BreakStmt, line_) {}
    void accept(ASTVisitor& visitor) override;
};

struct ContinueStmt : public Statement {
    explicit ContinueStmt(int line_) : Statement(NodeType::ContinueStmt, line_) {}
    void accept(ASTVisitor& visitor) override;
};

struct PrintStmt : public Statement {
    std::unique_ptr<Expression> expr;

    explicit PrintStmt(std::unique_ptr<Expression> expr_, int line_) : Statement(NodeType::PrintStmt, line_), expr(std::move(expr_)) {}
    void accept(ASTVisitor& visitor) override;
};

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual void visitBinaryExpr(BinaryExpr& expr) = 0;
    virtual void visitUnaryExpr(UnaryExpr& expr) = 0;
    virtual void visitLiteralExpr(LiteralExpr& expr) = 0;
    virtual void visitLogicalExpr(LogicalExpr& expr) = 0;
    virtual void visitVariableExpr(VariableExpr& expr) = 0;
    virtual void visitInputExpr(InputExpr& expr) = 0;
    virtual void visitAssignExpr(AssignExpr& expr) = 0;
    virtual void visitUpdateExpr(UpdateExpr& expr) = 0;

    virtual void visitBlockStmt(BlockStmt& stmt) = 0;
    virtual void visitExpressionStmt(ExpressionStmt& stmt) = 0;
    virtual void visitLetStmt(LetStmt& stmt) = 0;
    virtual void visitIfStmt(IfStmt& stmt) = 0;
    virtual void visitWhileStmt(WhileStmt& stmt) = 0;
    virtual void visitForStmt(ForStmt& stmt) = 0;
    virtual void visitBreakStmt(BreakStmt& stmt) = 0;
    virtual void visitContinueStmt(ContinueStmt& stmt) = 0;
    virtual void visitPrintStmt(PrintStmt& stmt) = 0;
};
