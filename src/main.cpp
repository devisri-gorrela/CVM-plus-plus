/**
 * @file main.cpp
 * @brief CVM++ entry point — REPL, script runner, and CLI interface.
 *
 * Provides three execution modes: interactive REPL with persistent state and
 * optional bytecode disassembly, single-file script execution, and bytecode
 * dump mode for debugging. Orchestrates the full compilation pipeline:
 * source -> Lexer -> Parser -> Compiler -> (optional Disassembler) -> VM.
 */
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "vm.h"
#include "disasm.h"
#include <iostream>
#include <fstream>
#include <sstream>

static constexpr const char* CVM_VERSION = "1.1.0";

/// Prints the CVM++ welcome banner shown at REPL startup.
void printBanner() {
    std::cout << "\n"
              << "  CVM++ v" << CVM_VERSION << " - Stack-based VM & Compiler\n"
              << "  ===========================================\n"
              << "  > Type 'exit' to quit.\n"
              << "  > Type 'disasm on' to view bytecode.\n\n";
}

struct RunOptions {
    bool dumpBytecode = false;  // REPL: Show disassembly alongside execution.
    bool dumpOnly = false;      // CLI: disassemble without executing.
};

void run(const std::string& source, Compiler& compiler, VM& vm, const RunOptions& opts) {
    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        Parser parser(tokens);
        auto statements = parser.parse();

        Chunk chunk = compiler.compile(statements);

        if (opts.dumpBytecode || opts.dumpOnly) {
            disassembleChunk(chunk, "Compiled Bytecode");
        }

        if (!opts.dumpOnly) {
            vm.execute(chunk);
        }
    } catch (const LexError& e) {
        std::cerr << "Error: " << e.what() << '\n';
    } catch (const ParseError& e) {
        std::cerr << "Error: " << e.what() << '\n';
    } catch (const CompileError& e) {
        std::cerr << "Error: " << e.what() << '\n';
    } catch (const RuntimeError& e) {
        std::cerr << "Error: " << e.what() << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        compiler.reset();
        vm.reset();
    }
}

void repl() {
    printBanner();
    VM vm;
    Compiler compiler;
    RunOptions opts;
    std::string line;
    while (true) {
        std::cout << "cvm> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "exit" || line == "quit") break;
        if (line == "disasm on") {
            opts.dumpBytecode = true;
            std::cout << "  [*] Bytecode disassembly enabled.\n";
            continue;
        }
        if (line == "disasm off") {
            opts.dumpBytecode = false;
            std::cout << "  [*] Bytecode disassembly disabled.\n";
            continue;
        }
        if (line.empty()) continue;
        
        run(line, compiler, vm, opts);
    }
}

void runFile(const std::string& path, const RunOptions& opts) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << path << '\n';
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    VM vm;
    Compiler compiler;
    run(buffer.str(), compiler, vm, opts);
}

void printUsage() {
    std::cout << "\n"
              << "  CVM++ v" << CVM_VERSION << "  --  Stack-based Virtual Machine & Compiler\n"
              << "  =============================================================\n"
              << "\n"
              << "  USAGE:    cvm [options] [script.cvm]\n"
              << "\n"
              << "  OPTIONS:\n"
              << "    --dump ........... Disassemble bytecode without executing\n"
              << "    --help, -h ....... Show this help message\n"
              << "    --version, -v .... Show version information\n"
              << "\n"
              << "  EXAMPLES:\n"
              << "    cvm                    Start the interactive REPL\n"
              << "    cvm script.cvm         Run a script file\n"
              << "    cvm --dump script.cvm  Disassemble a script\n"
              << "\n"
              << "  FEATURES:\n"
              << "    Integer arithmetic  |  Bitwise ops      |  Block scoping\n"
              << "    if/else/while/for   |  break/continue   |  input/print\n"
              << "    Constant folding    |  Dead-code elim.  |  Direct threading\n"
              << "\n";
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        repl();
    } else {
        RunOptions opts;
        std::string path;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--dump") {
                opts.dumpOnly = true;
            } else if (arg == "--help" || arg == "-h") {
                printUsage();
                return 0;
            } else if (arg == "--version" || arg == "-v") {
                std::cout << "CVM++ v" << CVM_VERSION << "\n";
                return 0;
            } else {
                path = arg;
            }
        }
        if (path.empty()) {
            std::cerr << "Error: No script file specified.\n";
            printUsage();
            return 1;
        }
        runFile(path, opts);
    }
    return 0;
}
