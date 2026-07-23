/**
 * @file ast.cpp
 * @brief AST visitor accept() dispatch implementations.
 *
 * Each accept() method delegates to the corresponding visit method on the
 * provided ASTVisitor, enabling double-dispatch without RTTI. This separates
 * the traversal logic from the node definitions while maintaining type safety.
 */

#include "ast.h"


void BinaryExpr::accept(ASTVisitor& visitor) { visitor.visitBinaryExpr(*this); }
void UnaryExpr::accept(ASTVisitor& visitor) { visitor.visitUnaryExpr(*this); }
void LogicalExpr::accept(ASTVisitor& visitor) { visitor.visitLogicalExpr(*this); }
void LiteralExpr::accept(ASTVisitor& visitor) { visitor.visitLiteralExpr(*this); }
void VariableExpr::accept(ASTVisitor& visitor) { visitor.visitVariableExpr(*this); }
void InputExpr::accept(ASTVisitor& visitor) { visitor.visitInputExpr(*this); }
void AssignExpr::accept(ASTVisitor& visitor) { visitor.visitAssignExpr(*this); }
void UpdateExpr::accept(ASTVisitor& visitor) { visitor.visitUpdateExpr(*this); }


void BlockStmt::accept(ASTVisitor& visitor) { visitor.visitBlockStmt(*this); }
void ExpressionStmt::accept(ASTVisitor& visitor) { visitor.visitExpressionStmt(*this); }
void LetStmt::accept(ASTVisitor& visitor) { visitor.visitLetStmt(*this); }
void IfStmt::accept(ASTVisitor& visitor) { visitor.visitIfStmt(*this); }
void WhileStmt::accept(ASTVisitor& visitor) { visitor.visitWhileStmt(*this); }
void ForStmt::accept(ASTVisitor& visitor) { visitor.visitForStmt(*this); }
void BreakStmt::accept(ASTVisitor& visitor) { visitor.visitBreakStmt(*this); }
void ContinueStmt::accept(ASTVisitor& visitor) { visitor.visitContinueStmt(*this); }
void PrintStmt::accept(ASTVisitor& visitor) { visitor.visitPrintStmt(*this); }
