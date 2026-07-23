cmake_minimum_required(VERSION 3.10)
project(CVM_PlusPlus VERSION 1.1.0 LANGUAGES CXX)

# ---- C++ Standard ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Build Type ----
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (Debug/Release/RelWithDebInfo)" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ---- Source Files ----
set(CVM_SOURCES
    src/main.cpp
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/compiler.cpp
    src/vm.cpp
    src/disasm.cpp
)

set(CVM_HEADERS
    src/token.h
    src/error.h
    src/opcode.h
    src/chunk.h
    src/lexer.h
    src/ast.h
    src/parser.h
    src/compiler.h
    src/vm.h
    src/disasm.h
)

# ---- Main Executable ----
add_executable(cvm ${CVM_SOURCES} ${CVM_HEADERS})
target_include_directories(cvm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ---- Compiler Warnings ----
target_compile_options(cvm PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wshadow -Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)

# ---- Release Optimizations ----
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_options(cvm PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-O2 -DNDEBUG>
        $<$<CXX_COMPILER_ID:MSVC>:/O2 /DNDEBUG>
    )
endif()
