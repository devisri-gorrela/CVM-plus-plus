# Contributing to CVM++

Thank you for your interest in CVM++! We welcome contributions ranging from bug fixes to new language features and VM optimizations.

## Development Workflow

1. **Fork and Clone**: Fork the repository and clone your fork locally.
2. **Branch**: Create a new branch for your feature or bug fix (`git checkout -b feature/new-opcode`).
3. **Build**: Ensure you can build the project using CMake:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
4. **Develop**: Make your changes. If you are adding a new AST node, ensure it is integrated into the single-pass compiler and the Disassembler.
5. **Test**: Run the entire regression suite to ensure no existing behavior is broken.
   ```bash
   # Linux/macOS
   for f in tests/*.cvm; do ./build/cvm "$f"; done
   ```
6. **Format**: Please format your C++ code using the provided `.clang-format` file before committing.

## Architectural Guidelines

- **Zero RTTI**: We avoid `dynamic_cast`. If you add a new AST node, define it in the `NodeType` enum.
- **VM Performance**: The virtual machine uses direct threading (computed gotos) on GCC/Clang and pre-allocates the variables vector to `MAX_VARIABLES` to strip dynamic bounds-checking from the hot path. Avoid introducing dynamic memory allocations or costly branches inside the main VM execution loop (`VM::execute`).
- **Error Handling**: Throw specific exceptions (`LexError`, `ParseError`, `CompileError`, `RuntimeError`) carrying line and column context rather than generic runtime exceptions.

## Pull Request Process

- Ensure your code compiles cleanly with `-Wall -Wextra -Wpedantic -Werror`.
- Add a new `.cvm` test file in the `tests/` directory for any new feature.
- Provide a clear description of the problem solved and the performance implications (if any) in your PR.
