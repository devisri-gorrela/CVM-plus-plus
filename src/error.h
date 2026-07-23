#pragma once
#include <stdexcept>
#include <string>

// Structured error hierarchy for programmatic error handling.
// All CVM++ errors derive from CVMError, which carries line/column context.

class CVMError : public std::runtime_error {
public:
    int line;
    int column;

    CVMError(const std::string& phase, const std::string& msg, int line_ = 0, int col = 0)
        : std::runtime_error(formatMessage(phase, msg, line_, col)),
          line(line_), column(col), rawMsg(msg) {}

    const std::string& getRawMsg() const { return rawMsg; }

private:
    std::string rawMsg;

private:
    static std::string formatMessage(const std::string& phase, const std::string& msg, int line_, int col) {
        std::string result = phase + " Error";
        if (line_ > 0) {
            result += " [line " + std::to_string(line_);
            if (col > 0) result += ":" + std::to_string(col);
            result += "]";
        }
        result += ": " + msg;
        return result;
    }
};

class LexError : public CVMError {
public:
    LexError(const std::string& msg, int line_ = 0, int col = 0)
        : CVMError("Lexer", msg, line_, col) {}
};

class ParseError : public CVMError {
public:
    ParseError(const std::string& msg, int line_ = 0, int col = 0)
        : CVMError("Parser", msg, line_, col) {}
};

class CompileError : public CVMError {
public:
    CompileError(const std::string& msg, int line_ = 0, int col = 0)
        : CVMError("Compile", msg, line_, col) {}
};

class RuntimeError : public CVMError {
public:
    RuntimeError(const std::string& msg, int line_ = 0, int col = 0)
        : CVMError("Runtime", msg, line_, col) {}
};
