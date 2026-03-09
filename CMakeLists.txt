cmake_minimum_required(VERSION 3.16)
project(DictatorScript LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Collect all source files
set(COMPILER_SOURCES
    src/compiler/Token.cpp
    src/compiler/Lexer.cpp
    src/compiler/ASTNode.cpp
    src/compiler/Parser.cpp
    src/compiler/SymbolTable.cpp
    src/compiler/SemanticAnalyzer.cpp
    src/compiler/CodeGenerator.cpp
    src/compiler/ErrorReporter.cpp
    src/compiler/Compiler.cpp
)

# Main executable
add_executable(dsc
    src/main.cpp
    ${COMPILER_SOURCES}
)

target_include_directories(dsc PRIVATE src)

# Compiler warnings
if(MSVC)
    target_compile_options(dsc PRIVATE /W4)
else()
    target_compile_options(dsc PRIVATE -Wall -Wextra -Wpedantic)
endif()
