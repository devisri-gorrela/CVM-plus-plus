---
name: Bug Report
about: Create a report to help us improve CVM++
title: "[BUG] "
labels: bug
assignees: ''
---

**Describe the bug**
A clear and concise description of what the bug is (e.g., Compiler crash, VM stack underflow, incorrect constant folding).

**To Reproduce**
Please provide the minimal `.cvm` source code that reproduces the issue:
```javascript
// Paste CVM++ code here
```

**Expected behavior**
What did you expect to happen?

**Environment (please complete the following information):**
- OS: [e.g. Ubuntu 22.04, Windows 11]
- Compiler used to build CVM++: [e.g. GCC 11, Clang 14, MSVC 2022]
- Branch/Commit: [e.g. main, or specific commit hash]

**Disassembly Dump**
If the code compiles but executes incorrectly, please provide the output of `./build/cvm --dump your_script.cvm`:
```plaintext
// Paste bytecode dump here
```
