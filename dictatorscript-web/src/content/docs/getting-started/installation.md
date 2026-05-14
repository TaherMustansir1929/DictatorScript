---
title: Installation
description: Learn how to build the DictatorScript compiler.
---

The DictatorScript compiler (`dsc`) is built using C++ and CMake. It is a transpiler that converts DictatorScript (`.ds`) source code into standard C++ and then invokes a C++ compiler to produce an executable.

## Prerequisites

To build the compiler, you need:

- **CMake 3.16+**
- A C++17 compliant compiler (e.g., `g++`, `clang++`, or `MSVC`)

## Building from Source

First, clone the repository containing the DictatorScript source code:

```bash
git clone https://github.com/TaherMustansir1929/DictatorScript.git
cd DictatorScript
```

Then, configure and build the project using CMake:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Alternatively, you can use the newer CMake invocation:

```bash
cmake -S . -B build
cmake --build build
```

This will produce the `dsc` executable in the build directory.

## Usage

You can use the `dsc` compiler to compile `.ds` files:

```bash
dsc source.ds                    # compile and produce executable
dsc source.ds -o myprogram       # compile to named binary
dsc source.ds --emit-cpp         # only emit .cpp, don't invoke g++
dsc source.ds --dump-tokens      # debug: print token stream
dsc source.ds --dump-ast         # debug: print AST
dsc source.ds --verbose          # print each pipeline stage
```
