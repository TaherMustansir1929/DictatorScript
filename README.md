# DictatorScript Compiler (dsc)

A transpiling compiler for **DictatorScript** (`.ds`), a custom educational programming language inspired by Go's simplicity, mapped onto C++ fundamentals. DictatorScript source code is parsed and translated into valid C++ source code, which is then compiled by a standard C++ compiler to produce an executable.

## Building

Requires CMake 3.16+ and a C++17 compiler (g++, clang++, or MSVC).

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

or

```bash
cmake -S . -B build
cmake --build build
```

This produces the `dsc` executable.

## Usage

```bash
dsc source.ds                    # compile and produce executable
dsc source.ds -o myprogram       # compile to named binary
dsc source.ds --emit-cpp         # only emit .cpp, don't invoke g++
dsc source.ds --dump-tokens      # debug: print token stream
dsc source.ds --dump-ast         # debug: print AST
dsc source.ds --verbose          # print each pipeline stage
```

## Language Reference

### Entry Point

Every program must have exactly one `regime start` block:

```
regime start {
    // program body
}
```

### Variable Declarations

All variables must be declared with `declare`. Uninitialized variables are auto-zero-initialized.

```
declare int x = 5
declare float pi = 3.14
declare bool flag = loyal         // true
declare elite string name = "Z"  // const
declare int y                    // auto-initialized to 0
```

### Data Types

| DictatorScript | C++ Equivalent |
|---|---|
| `int` | `int` |
| `float` | `float` |
| `double` | `double` |
| `char` | `char` |
| `string` | `std::string` |
| `bool` | `bool` |
| `void` | `void` |
| `int[]` | `std::vector<int>` |
| `int->` | `int*` |
| `map<K,V>` | `std::unordered_map<K,V>` |

### Boolean Literals

- `loyal` = `true`
- `traitor` = `false`

### Structs

```
law Point {
    int x
    int y
}
```

### Functions

```
command add(int a, int b) -> int {
    report a + b
}

command greet(string name) -> void {
    broadcast("Hello " + name + "\n")
}
```

### Control Flow

```
interrogate (x > 0) {
    broadcast("positive\n")
} otherwise interrogate (x < 0) {
    broadcast("negative\n")
} otherwise {
    broadcast("zero\n")
}
```

### Loops

```
impose (int i from [1..5]) { }          // range for (inclusive)
impose (int x from arr) { }             // for-each
impose (condition) { }                  // while
impose forever { }                      // infinite loop
```

- `silence` = `break`
- `proceed` = `continue`

### I/O

```
broadcast("Hello " + name + "\n")       // print
demand(x)                               // read input
```

### Pointers and Memory

```
declare int-> ptr = @x                  // address-of
declare int val = ^ptr                  // dereference
declare int-> heap = summon int         // new
kill heap                               // delete
```

### Arrays

```
declare int[] nums = [1, 2, 3]
declare int[] range = [0..10]
nums.add(5)
nums.remove(0)
declare int len = nums.size()
```

### Hashmaps

```
declare map<string, int> scores
scores["Alice"] = 95
```

### Imports

```
import "utils"                          // imports utils.ds
```

### Comments

```
// single-line comment
/* multi-line
   comment */
```

## Compiler Architecture

```
.ds source → Lexer → Parser → SemanticAnalyzer → CodeGenerator → .cpp → g++ → executable
```

Each stage is a separate C++ class with clear responsibilities:

- **Lexer** — Tokenizes source into a token stream
- **Parser** — Recursive descent parser producing an AST
- **SemanticAnalyzer** — Type checking, scope validation (visitor pattern)
- **CodeGenerator** — Emits C++ source code (visitor pattern)
- **Compiler** — Orchestrates the pipeline

## Project Structure

```
DictatorScript/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp
│   └── compiler/
│       ├── Compiler.h / .cpp
│       ├── ErrorReporter.h / .cpp
│       ├── Token.h / .cpp
│       ├── TokenType.h
│       ├── Lexer.h / .cpp
│       ├── ASTNode.h / .cpp
│       ├── Parser.h / .cpp
│       ├── SymbolTable.h / .cpp
│       ├── SemanticAnalyzer.h / .cpp
│       └── CodeGenerator.h / .cpp
├── examples/
│   ├── hello_world.ds
│   ├── sample.ds
│   └── multifile/
└── tests/
```
