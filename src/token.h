/**
 * @file token.h
 * @brief Token type definitions and Token structure for the CVM++ lexer.
 *
 * Defines the complete set of token types recognized by the lexer, including
 * keywords, operators, symbols, literals, and identifiers. Each token carries
 * its type, string value, and source location (line, column) for accurate
 * error reporting and debugging throughout the compilation pipeline.
 */
#pragma once
#include <string>
#include <cstdint>

enum class TokenType {
    // Keywords
    LET, IF, ELSE, WHILE, FOR, BREAK, CONTINUE, PRINT, INPUT, TRUE_LIT, FALSE_LIT,

    // Operators
    PLUS, MINUS, STAR, SLASH, MOD, PLUS_PLUS, MINUS_MINUS,
    EQUAL, EQ_EQ, BANG_EQ, LESS, GREATER, LESS_EQ, GREATER_EQ,
    BIT_XOR, BIT_AND, BIT_OR, SHL, SHR, BIT_NOT, NOT,
    AND_AND, OR_OR,

    // Symbols
    LPAREN, RPAREN, LBRACE, RBRACE, SEMICOLON,

    // Literals & Identifiers
    NUMBER, IDENTIFIER,

    // End of file
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
};
