/**
 * @file lexer.h
 * @brief Lexer/tokenizer class declaration for the CVM++ language.
 * -
 * The Lexer converts raw source code strings into a vector of Tokens, handling
 * whitespace, single-line (//) and block ((slash-star ... star-slash) comments, numeric literals
 * with hex (0x), binary (0b), and octal (0o) prefixes, multi-character
 * operators, and 11 reserved keywords. Tracks line and column information
 * for precise error reporting in later compilation stages.
 */

#pragma once
#include "token.h"
#include <vector>
#include <string>

class Lexer {
public:
    Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    const std::string& source;
    size_t pos;
    int line;
    int column;

    [[nodiscard]] char peek() const;
    [[nodiscard]] char peekNext() const;
    char advance();
    [[nodiscard]] bool isAtEnd() const;

    void skipWhitespace();
    Token createToken(TokenType type, const std::string& value);
};
