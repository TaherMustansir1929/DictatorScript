---

## DictatorScript Compiler in C++

---

### PROJECT OVERVIEW

You are tasked with building a **compiler for a custom programming language called DictatorScript** (file extension: `.ds`). The compiler must be written in **C++ as a CMake project**. The compilation strategy is **transpilation**: DictatorScript source code is parsed and translated into valid C++ source code, which is then compiled by a standard C++ compiler (e.g., `g++` or `clang++`) to produce an executable.

DictatorScript is a **hobby/educational language** inspired by the simplicity and semantics of Go/Golang, but mapped onto C++ fundamentals. It is intentionally **not production-grade** — keep complexity proportional to a well-structured student project. The language covers all fundamental C++ features **except Object-Oriented Programming** (no classes, inheritance, virtual functions, access specifiers, or templates). The design philosophy is: simplified, opinionated, fun imperative vocabulary that maps 1-to-1 with C++ constructs.

---

### COMPILER ARCHITECTURE REQUIREMENTS

The compiler must be structured as **separate C++ classes**, one per compiler stage. Each stage must be independently testable and loosely coupled so that new language features can be added with minimal changes to other stages. The pipeline is:

```
.ds source file(s)
      ↓
  [Lexer / Tokenizer]
      ↓
  [Parser / AST Builder]
      ↓
  [Semantic Analyzer]
      ↓
  [Code Generator]
      ↓
  .cpp output file(s)
      ↓
  [C++ Compiler invocation (g++ / clang++)]
      ↓
  Executable
```

**Required classes and their responsibilities:**

1. **`Lexer`** — Reads raw `.ds` source text, produces a flat stream of `Token` objects. Handles whitespace, comments (`//` single-line, `/* */` multi-line), string/char literals, numeric literals, identifiers, and all DictatorScript keywords and symbols.

2. **`Token`** — A plain struct/class holding: `TokenType` (enum), `std::string lexeme`, `int line`, `int column`. Provide a `toString()` method for debug output.

3. **`TokenType`** — A comprehensive `enum class` covering every keyword, operator, punctuation, and literal type in DictatorScript (full list provided below).

4. **`Parser`** — Consumes the token stream, produces an **Abstract Syntax Tree (AST)**. Use recursive descent parsing. Each grammar rule maps to a parse method (e.g., `parseStatement()`, `parseExpression()`, `parseFunctionDecl()`, etc.).

5. **`ASTNode`** — A base class (or variant/union) for all AST node types. Use an inheritance hierarchy or `std::variant`. Every major language construct (declarations, expressions, statements, function definitions, struct definitions, etc.) must have its own node type. Nodes must support a `print()` or `dump()` method for debugging the tree.

6. **`SemanticAnalyzer`** — Walks the AST and performs: symbol table management (scoped), type checking, undeclared variable detection, function signature validation, `report` return type checking, duplicate declaration checking, and `const` (`elite`) mutation checks. Emit clear, line-numbered error messages. This stage should be a **visitor pattern** over the AST.

7. **`SymbolTable`** — A scoped symbol table (stack of maps). Supports entering/exiting scopes and looking up symbols through the scope chain.

8. **`CodeGenerator`** — Walks the validated AST and emits C++ source code as a string or file stream. This should also use the **visitor pattern**. The generator must produce clean, readable C++ (not obfuscated). It should emit standard `#include` headers as needed (`<iostream>`, `<string>`, `<vector>`, `<unordered_map>`, etc.).

9. **`Compiler`** — Orchestrator class that chains all stages together, manages file I/O, invokes `g++`/`clang++` via `std::system()` or `popen()` on the generated `.cpp`, and handles overall error propagation.

10. **`ErrorReporter`** — Central error/warning reporting utility used by all stages. Collects errors with file name, line, column, and message. Supports a `hasErrors()` check to abort the pipeline early.

---

### DICTATORSCRIPT LANGUAGE SPECIFICATION

#### 1. TOKEN TYPE ENUM

Generate the following `enum class TokenType` (add any additional tokens you identify as necessary):

```
// Literals
TOKEN_INT_LITERAL, TOKEN_FLOAT_LITERAL, TOKEN_CHAR_LITERAL,
TOKEN_STRING_LITERAL, TOKEN_BOOL_LITERAL,

// Identifiers
TOKEN_IDENTIFIER,

// Keywords — Types
KW_INT, KW_FLOAT, KW_DOUBLE, KW_CHAR, KW_STRING, KW_BOOL, KW_VOID,

// Keywords — Declarations
KW_DECLARE, KW_ELITE, KW_LAW, KW_COMMAND,

// Keywords — Entry Point
KW_REGIME,

// Keywords — Boolean Literals
KW_LOYAL,   // true
KW_TRAITOR, // false

// Keywords — Control Flow
KW_INTERROGATE,   // if
KW_OTHERWISE,     // else / else if
KW_IMPOSE,        // for / while
KW_FROM,          // range/iteration keyword
KW_FOREVER,       // infinite loop
KW_SILENCE,       // break
KW_PROCEED,       // continue
KW_REPORT,        // return

// Keywords — I/O
KW_BROADCAST,     // cout / print
KW_DEMAND,        // cin / input

// Keywords — Memory
KW_SUMMON,        // new  (dynamic allocation)
KW_KILL,          // delete (free dynamic memory)
KW_NULL_VAL,      // nullptr

// Keywords — Multi-file / Imports
KW_ENLIST,        // #include / import for other .ds files

// Keywords — Hashmaps
KW_MAP,           // map/hashmap type keyword

// Operators — Arithmetic
OP_PLUS, OP_MINUS, OP_STAR, OP_SLASH, OP_PERCENT,

// Operators — Comparison
OP_EQ, OP_NEQ, OP_LT, OP_GT, OP_LTE, OP_GTE,

// Operators — Logical
OP_AND, OP_OR, OP_NOT,

// Operators — Assignment
OP_ASSIGN, OP_PLUS_ASSIGN, OP_MINUS_ASSIGN,
OP_STAR_ASSIGN, OP_SLASH_ASSIGN,

// Operators — Pointer / Address
OP_AT,            // @ — address-of (replaces &)
OP_ARROW,         // -> — return type annotation & pointer dereference
OP_DEREF,         // ^ — dereference operator (replaces *)

// Punctuation
PUNCT_LBRACE, PUNCT_RBRACE,     // { }
PUNCT_LPAREN, PUNCT_RPAREN,     // ( )
PUNCT_LBRACKET, PUNCT_RBRACKET, // [ ]
PUNCT_SEMICOLON,                // ; (optional: make newline-sensitive like Go)
PUNCT_COLON,                    // :
PUNCT_COMMA,                    // ,
PUNCT_DOT,                      // .
PUNCT_DOTDOT,                   // .. (range operator)

// Special
TOKEN_EOF,
TOKEN_NEWLINE,
TOKEN_UNKNOWN
```

---

#### 2. KEYWORD MAPPING TABLE

| DictatorScript | C++ Equivalent | Notes |
|---|---|---|
| `law` | `struct` | struct definition |
| `command` | function definition | user-defined function |
| `regime start` | `int main()` | program entry point |
| `declare` | variable declaration | explicit declaration keyword |
| `elite` | `const` | constant modifier |
| `loyal` | `true` | boolean true |
| `traitor` | `false` | boolean false |
| `interrogate` | `if` | |
| `otherwise interrogate` | `else if` | |
| `otherwise` | `else` | |
| `impose (x from [a..b])` | `for (int x = a; x <= b; x++)` | range-based for |
| `impose (x from arr)` | `for (auto x : arr)` | array iteration |
| `impose forever` | `while (true)` | infinite loop |
| `impose (condition)` | `while (condition)` | while loop |
| `silence` | `break` | |
| `proceed` | `continue` | |
| `report` | `return` | |
| `broadcast(...)` | `std::cout << ... << std::endl` | print to stdout |
| `demand(x)` | `std::cin >> x` / `std::getline` | read from stdin |
| `@x` | `&x` | address-of operator |
| `int->` | `int*` | pointer type |
| `^ptr` | `*ptr` | dereference |
| `summon Type(args)` | `new Type(args)` | heap allocation |
| `kill ptr` | `delete ptr` | free heap memory |
| `enlist "file"` | `#include "file.ds"` (transpiled) | import another `.ds` file |
| `map<K,V>` | `std::unordered_map<K,V>` | hashmap type |
| `.add(x)` | `.push_back(x)` | array append |
| `.remove(i)` | `.erase(arr.begin() + i)` | array remove by index |
| `.size()` | `.size()` | length |
| `int[]` | `std::vector<int>` | dynamic array |
| `[0..10]` | range-init (generates loop or initializer list) | range literal |
| `null` | `nullptr` | null pointer |
| `//` | `//` | single-line comment |
| `/* */` | `/* */` | multi-line comment |

---

#### 3. DATA TYPES

Support the following primitive and compound types:

- **Primitives:** `int`, `float`, `double`, `char`, `bool`, `string` (maps to `std::string`)
- **Void:** `void` (for functions with no return value)
- **Arrays:** `Type[]` → `std::vector<Type>` (all arrays are dynamic)
- **Pointers:** `Type->` → `Type*` (use `@` for address-of, `^` for dereference)
- **Hashmaps:** `map<KeyType, ValueType>` → `std::unordered_map<KeyType, ValueType>`
- **Structs:** Defined with `law`, fields are typed and newline-separated (no semicolons inside struct body). Struct instances can be stack or heap allocated.
- **Auto-zero initialization:** Any `declare` without an initializer is zero-initialized (0, 0.0, `'\0'`, `false`, `""`) — mirroring Go semantics. Never leave uninitialized variables.

---

#### 4. LANGUAGE FEATURES IN DETAIL

**Variable Declarations**
```
declare int x = 5
declare float pi = 3.14
declare bool flag = loyal
declare elite string name = "ZeoXD"   // const
declare int xyz                        // auto zero-initialized to 0
```
- `declare` is mandatory for all variable declarations.
- `elite` makes the variable `const`.
- Type inference is NOT supported (explicit types required).

**Structs (`law`)**
```
law Point {
    int x
    int y
}
```
- Struct fields are separated by newlines (no semicolons).
- Structs can be used as function parameters, return types, and array element types.
- Struct literals use `{value1, value2, ...}` initializer syntax.

**Functions (`command`)**
```
command add(int a, int b) -> int {
    report a + b
}
command greet(string name) -> void {
    broadcast("Hello, " + name)
}
```
- `command` declares a function.
- `->` specifies return type (required, use `-> void` for no return).
- `report` is the return statement.
- Functions support multiple parameters.
- Functions must be declared before use OR the code generator must emit forward declarations automatically.

**Entry Point**
```
regime start {
    // program body
}
```
- There is exactly one `regime start` block per program.
- It transpiles to `int main() { ... return 0; }`.

**Control Flow**
```
interrogate (x > 0) {
    broadcast("positive")
} otherwise interrogate (x < 0) {
    broadcast("negative")
} otherwise {
    broadcast("zero")
}
```

**Loops (`impose`)**
```
// Range-based for loop (inclusive range)
impose (int i from [1..5]) { }

// Iterate over array
impose (int x from arr) { }

// Iterate over struct array (enhanced for)
impose (Student s from students) { }

// While loop
impose (a < b) { }

// Infinite loop
impose forever { }
```
- All loop variants use the `impose` keyword.
- `silence` = `break`, `proceed` = `continue`.
- Range `[a..b]` is **inclusive** on both ends.

**Arrays**
```
declare int[] nums = [1, 2, 3, 4]
declare int[] range = [0..10]         // initializes with 0,1,2,...,10
nums.add(5)
nums.remove(0)
declare int len = nums.size()
```
- All arrays are `std::vector<T>`.
- `[a..b]` range literals in declarations should generate a vector populated via a loop in the transpiled C++.
- Support indexing: `arr[i]`.

**Hashmaps**
```
declare map<string, int> scores
scores["Alice"] = 95
declare int aliceScore = scores["Alice"]
```
- Maps to `std::unordered_map<K,V>`.
- Support bracket access, insertion, and lookup.

**Pointers & Dynamic Memory**
```
declare int-> ptr = @x          // pointer to x
declare int val = ^ptr          // dereference
declare int-> heap = summon int // heap allocation (new int)
kill heap                       // delete heap
```
- `@` replaces `&` (address-of).
- `^` replaces `*` (dereference), except in type declarations where `->` denotes pointer type.
- `summon` replaces `new`, `kill` replaces `delete`.
- Pointer-to-struct: `declare Point-> p = summon Point`

**I/O**
```
broadcast("Hello " + name + "\n")   // cout
demand(x)                            // cin >> x (scalar)
demand(line)                         // getline for strings
```
- `broadcast` accepts a single expression (concatenation/formatting handled by transpiling to `<<` chains).
- `demand` detects the variable type: uses `std::getline` for `string`, `std::cin >>` for all others, and a generated `for` loop for arrays.

**Multi-file Support (`enlist`)**
```
enlist "math_utils"    // imports math_utils.ds
enlist "student"       // imports student.ds
```
- The compiler should resolve `.ds` file imports, compile them, and `#include` the generated `.cpp`/`.h` in the output.
- Each `.ds` file compiles to a `.h` + `.cpp` pair (declarations and definitions).
- The `regime start` file is the entry point and compiles to `main.cpp`.
- Include guards must be auto-generated.

---

#### 5. SAMPLE CODE (REFERENCE IMPLEMENTATION)

The following sample demonstrates all major features. The code generator must be able to correctly transpile this:

```ds
law Student {
    string name
    int marks
    char grade
    bool is_pass
}

command register_student(string name, int marks, char grade, bool is_pass) -> Student {
    Student s = {name, marks, grade.toUppercase(), is_pass}
    report s
}

regime start {
    declare int a = 6
    declare int b = 7
    declare float c = a / b

    declare bool is_true = loyal
    is_true = traitor

    declare int[] arr = [1,2,3,4]
    declare int[] iter = [0..10]

    declare int-> ptr = @a

    declare int xyz

    declare elite int vip = 666

    declare elite string f_name = "ZeoXD"

    broadcast("Hello world " + f_name + "!\n")
    demand(xyz)

    declare Student[] students
    students.add(register_student("ZeoXD", 100, 'A', loyal))
    students.add(register_student("RandomUser", -67, 'F', traitor))

    impose (int i from [1..5]) {
        interrogate (a > b) {
            // do something
        } otherwise interrogate (i == 3) {
            silence
        } otherwise interrogate (i % 2 == 0) {
            proceed
        } otherwise {
            // do something else
        }
    }

    impose (int i from arr) {
    }
    impose (Student s from students) {
    }
    impose forever {
    }
    impose (a < b) {
    }

    declare int-> heap = summon int
    kill heap
}
```

**Expected transpiled C++ output (approximate):**
```cpp
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>

struct Student {
    std::string name;
    int marks;
    char grade;
    bool is_pass;
};

Student register_student(std::string name, int marks, char grade, bool is_pass) {
    Student s = {name, marks, (char)std::toupper(grade), is_pass};
    return s;
}

int main() {
    int a = 6;
    int b = 7;
    float c = a / b;

    bool is_true = true;
    is_true = false;

    std::vector<int> arr = {1, 2, 3, 4};
    std::vector<int> iter;
    for (int _i = 0; _i <= 10; _i++) iter.push_back(_i);

    int* ptr = &a;

    int xyz = 0;

    const int vip = 666;
    const std::string f_name = "ZeoXD";

    std::cout << "Hello world " << f_name << "!" << "\n";
    std::cin >> xyz;

    std::vector<Student> students;
    students.push_back(register_student("ZeoXD", 100, 'A', true));
    students.push_back(register_student("RandomUser", -67, 'F', false));

    for (int i = 1; i <= 5; i++) {
        if (a > b) {
        } else if (i == 3) {
            break;
        } else if (i % 2 == 0) {
            continue;
        } else {
        }
    }

    for (int i : arr) {}
    for (Student s : students) {}
    while (true) {}
    while (a < b) {}

    int* heap = new int;
    delete heap;

    return 0;
}
```

---

#### 6. BUILT-IN METHOD SUPPORT

The code generator must handle these built-in method calls by transpiling them to their C++ equivalents:

| DictatorScript | C++ |
|---|---|
| `arr.add(x)` | `arr.push_back(x)` |
| `arr.remove(i)` | `arr.erase(arr.begin() + i)` |
| `arr.size()` | `arr.size()` |
| `arr[i]` | `arr[i]` |
| `map.has(key)` | `map.count(key) > 0` |
| `map.remove(key)` | `map.erase(key)` |
| `str.size()` | `str.size()` |
| `str.toUppercase()` | conversion using `std::toupper` in a loop or `std::transform` |
| `str.toLowercase()` | conversion using `std::tolower` |
| `str.contains(sub)` | `str.find(sub) != std::string::npos` |
| `char.toUppercase()` | `(char)std::toupper(c)` |

---

### CMAKE PROJECT STRUCTURE

Generate a complete, buildable CMake project with this structure:

```
DictatorScript/
├── CMakeLists.txt                  (root, sets C++17, adds subdirectories)
├── README.md                       (usage instructions)
├── src/
│   ├── main.cpp                    (CLI entry: parses args, invokes Compiler)
│   ├── compiler/
│   │   ├── Compiler.h / .cpp       (orchestrator)
│   │   ├── ErrorReporter.h / .cpp
│   │   ├── Token.h / .cpp
│   │   ├── TokenType.h             (enum class only)
│   │   ├── Lexer.h / .cpp
│   │   ├── ASTNode.h / .cpp        (full AST node hierarchy)
│   │   ├── Parser.h / .cpp
│   │   ├── SymbolTable.h / .cpp
│   │   ├── SemanticAnalyzer.h / .cpp
│   │   └── CodeGenerator.h / .cpp
├── examples/
│   ├── hello_world.ds
│   ├── sample.ds                   (the provided sample code)
│   └── multifile/
│       ├── main.ds
│       └── utils.ds
└── tests/
    └── (optional: basic test cases per stage)
```

---

### CLI USAGE

The compiled `dsc` binary should support:

```bash
dsc source.ds                          # compile and run
dsc source.ds -o output                # compile to named binary
dsc source.ds --emit-cpp               # only emit .cpp, don't invoke g++
dsc source.ds --dump-tokens            # debug: print token stream
dsc source.ds --dump-ast               # debug: print AST
dsc source.ds --verbose                # print each pipeline stage
```

---

### ERROR HANDLING REQUIREMENTS

- All errors must include **filename, line number, and column number**.
- The pipeline must **abort after each stage** if errors exist (don't run the code generator on a broken AST).
- Error messages should be clear and beginner-friendly (this is an educational tool).
- Distinguish between **errors** (fatal) and **warnings** (non-fatal, e.g., unused variables).

Example:
```
[DictatorScript Error] sample.ds:14:5 — Undeclared variable 'xyz' used before declaration.
[DictatorScript Warning] sample.ds:22:3 — Variable 'c' declared but never used.
```

---

### IMPLEMENTATION GUIDELINES & CONSTRAINTS

1. **Use C++17** throughout. Use `std::variant`, `std::optional`, `std::string_view`, and structured bindings where appropriate.
2. **No third-party libraries.** Only the C++ standard library.
3. **Visitor pattern** is required for both the Semantic Analyzer and Code Generator walking the AST. Define a base `ASTVisitor` interface.
4. **Recursive descent parser** — do not use parser generators (no ANTLR, Bison, Flex).
5. **Newline sensitivity**: Statements are newline-terminated (like Go). Semicolons are optional and treated like newlines if present. The Lexer should emit `TOKEN_NEWLINE` and the Parser should handle statement boundaries accordingly.
6. **Forward declarations**: The Code Generator must emit C++ forward declarations for all structs and functions before their definitions to avoid ordering issues.
7. **String concatenation**: `+` on strings should transpile to `+` on `std::string` (which is valid in C++).
8. **Extensibility**: Every stage should be designed so that adding a new keyword, operator, or construct requires changes only in well-defined, localized places (e.g., adding a new `TokenType`, a new `ASTNode` subclass, a new visitor method). Comment the extension points clearly.
9. **Code comments**: Every class and non-trivial method must have a comment explaining its purpose and how to extend it.
10. **The generated C++ must compile cleanly** with `g++ -std=c++17 -Wall` with no errors. Test against the provided sample code as the primary acceptance criterion.

---

### DELIVERABLE CHECKLIST

Produce all of the following:
- [ ] Complete `TokenType.h` with full enum
- [ ] `Token.h/.cpp`
- [ ] `Lexer.h/.cpp` — fully functional tokenizer
- [ ] `ASTNode.h/.cpp` — complete node hierarchy for all language constructs
- [ ] `Parser.h/.cpp` — recursive descent parser producing a valid AST
- [ ] `SymbolTable.h/.cpp` — scoped symbol table
- [ ] `SemanticAnalyzer.h/.cpp` — type checking and validation via visitor
- [ ] `CodeGenerator.h/.cpp` — C++ emitter via visitor
- [ ] `ErrorReporter.h/.cpp`
- [ ] `Compiler.h/.cpp` — pipeline orchestrator
- [ ] `main.cpp` — CLI with all flags
- [ ] `CMakeLists.txt` — working CMake build
- [ ] `examples/sample.ds` — the provided sample code
- [ ] `README.md` — build instructions and language reference summary

---

Sample code provided in ./sample-code.txt