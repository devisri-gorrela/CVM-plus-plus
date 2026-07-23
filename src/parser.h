/**
 * @file parser.h
 * @brief Recursive-descent parser class declaration for CVM++.
 *
 * The Parser consumes a token vector and produces a typed AST using a
 * precedence-climbing approach with the full operator precedence chain:
 * assignment, logical OR/AND, bitwise OR/XOR/AND, equality, comparison,
 * shift, term, factor, unary, postfix, and primary. Supports error recovery
 * via synchronize() so parsing can continue after encountering syntax errors.
 */

#pragma once
#include "lexer.h"
#include "ast.h"
#include <vector>
#include <memory>

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::vector<std::unique_ptr<Statement>> parse();

private:
    const std::vector<Token>& tokens;
    size_t current;

    // Helpers
    [[nodiscard]] const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(std::initializer_list<TokenType> types);
    const Token& consume(TokenType type, const std::string& message);

    bool isAtEnd() const;

    void synchronize();

    // Declaration & Statement Parsing
    std::unique_ptr<Statement> declaration();
    std::unique_ptr<Statement> letDeclaration();
    std::unique_ptr<Statement> statement();
    std::unique_ptr<Statement> ifStatement();
    std::unique_ptr<Statement> forStatement();
    std::unique_ptr<Statement> whileStatement();
    std::unique_ptr<Statement> breakStatement();
    std::unique_ptr<Statement> continueStatement();
    std::unique_ptr<Statement> printStatement();
    std::unique_ptr<Statement> blockStatement();
    std::unique_ptr<Statement> expressionStatement();

    // Expression Parsing
    std::unique_ptr<Expression> expression();
    std::unique_ptr<Expression> assignment();
    std::unique_ptr<Expression> logicalOr();
    std::unique_ptr<Expression> logicalAnd();
    std::unique_ptr<Expression> bitwiseOr();
    std::unique_ptr<Expression> bitwiseXor();
    std::unique_ptr<Expression> bitwiseAnd();
    std::unique_ptr<Expression> equality();
    std::unique_ptr<Expression> comparison();
    std::unique_ptr<Expression> shift();
    std::unique_ptr<Expression> term();
    std::unique_ptr<Expression> factor();
    std::unique_ptr<Expression> unary();
    std::unique_ptr<Expression> postfix();
    std::unique_ptr<Expression> primary();
};
