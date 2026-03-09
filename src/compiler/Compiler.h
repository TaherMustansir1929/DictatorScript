#ifndef DICTATORSCRIPT_COMPILER_H
#define DICTATORSCRIPT_COMPILER_H

#include "ErrorReporter.h"
#include "ASTNode.h"
#include <string>
#include <set>

// Compiler is the pipeline orchestrator that chains all stages together:
//   1. Read .ds source file
//   2. Lex → token stream
//   3. Parse → AST
//   4. Resolve imports (recursively parse imported .ds files and merge ASTs)
//   5. Semantic analysis → validated AST
//   6. Code generation → C++ source
//   7. (Optional) Invoke g++/clang++ to compile the C++ source
//
// Extension point: to add new pipeline stages, insert them between
// existing stages in the compile() method.
class Compiler {
public:
    struct Options {
        std::string inputFile;
        std::string outputBinary;    // -o flag
        bool emitCppOnly = false;    // --emit-cpp
        bool dumpTokens = false;     // --dump-tokens
        bool dumpAST = false;        // --dump-ast
        bool verbose = false;        // --verbose
    };

    Compiler();

    // Compile a .ds source file according to the given options.
    // Returns 0 on success, non-zero on failure.
    int compile(const Options& options);

private:
    ErrorReporter errors_;

    // Read the entire contents of a file into a string.
    std::string readFile(const std::string& path);

    // Write a string to a file.
    bool writeFile(const std::string& path, const std::string& content);

    // Invoke the system C++ compiler on the generated .cpp file.
    int invokeCppCompiler(const std::string& cppFile, const std::string& outputBinary);

    // Resolve import statements by recursively parsing imported files
    // and merging their declarations into the program AST.
    // baseDir is the directory of the file containing the imports.
    // importedFiles tracks already-imported files to prevent circular imports.
    bool resolveImports(ProgramNode& program, const std::string& baseDir,
                        std::set<std::string>& importedFiles, bool verbose);

    // Get the directory portion of a file path.
    static std::string getDirectory(const std::string& filePath);

    // Normalize a file path for consistent comparison.
    static std::string normalizePath(const std::string& path);
};

#endif // DICTATORSCRIPT_COMPILER_H
