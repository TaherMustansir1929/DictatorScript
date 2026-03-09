#include "Compiler.h"
#include "Lexer.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"
#include "CodeGenerator.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

Compiler::Compiler() {}

std::string Compiler::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[DictatorScript Error] Could not open file: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool Compiler::writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "[DictatorScript Error] Could not write file: " << path << std::endl;
        return false;
    }
    file << content;
    return true;
}

int Compiler::invokeCppCompiler(const std::string& cppFile, const std::string& outputBinary) {
    std::string command = "g++ -std=c++17 -Wall -o \"" + outputBinary + "\" \"" + cppFile + "\"";
    std::cout << "[DictatorScript] Compiling C++: " << command << std::endl;
    int result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "[DictatorScript Error] C++ compilation failed." << std::endl;
    }
    return result;
}

int Compiler::compile(const Options& options) {
    errors_.clear();

    // ---- Stage 0: Read source file ----
    if (options.verbose) {
        std::cout << "[DictatorScript] Reading source file: " << options.inputFile << std::endl;
    }

    std::string source = readFile(options.inputFile);
    if (source.empty() && errors_.hasErrors()) {
        return 1;
    }

    // Extract filename for error reporting.
    std::string filename = options.inputFile;
    auto lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = filename.substr(lastSlash + 1);
    }

    // ---- Stage 1: Lexing ----
    if (options.verbose) {
        std::cout << "[DictatorScript] Stage 1: Lexing..." << std::endl;
    }

    Lexer lexer(source, filename, errors_);
    auto tokens = lexer.tokenize();

    if (errors_.hasErrors()) {
        errors_.printAll();
        return 1;
    }

    if (options.dumpTokens) {
        std::cout << "=== Token Stream ===" << std::endl;
        for (const auto& token : tokens) {
            if (token.type != TokenType::TOKEN_NEWLINE) {
                std::cout << token.toString() << std::endl;
            }
        }
        if (!options.dumpAST && !options.emitCppOnly) return 0;
    }

    // ---- Stage 2: Parsing ----
    if (options.verbose) {
        std::cout << "[DictatorScript] Stage 2: Parsing..." << std::endl;
    }

    Parser parser(tokens, filename, errors_);
    auto ast = parser.parse();

    if (errors_.hasErrors()) {
        errors_.printAll();
        return 1;
    }

    if (options.dumpAST) {
        std::cout << "=== Abstract Syntax Tree ===" << std::endl;
        if (ast) ast->print();
        if (!options.emitCppOnly) return 0;
    }

    // ---- Stage 3: Semantic Analysis ----
    if (options.verbose) {
        std::cout << "[DictatorScript] Stage 3: Semantic Analysis..." << std::endl;
    }

    SemanticAnalyzer analyzer(filename, errors_);
    if (ast) {
        analyzer.analyze(*ast);
    }

    if (errors_.hasErrors()) {
        errors_.printAll();
        return 1;
    }

    // Print warnings (non-fatal).
    if (errors_.hasWarnings()) {
        errors_.printAll();
    }

    // ---- Stage 4: Code Generation ----
    if (options.verbose) {
        std::cout << "[DictatorScript] Stage 4: Code Generation..." << std::endl;
    }

    CodeGenerator codegen(errors_);
    std::string cppCode = codegen.generate(*ast);

    if (errors_.hasErrors()) {
        errors_.printAll();
        return 1;
    }

    // Determine output file paths.
    std::string baseName = options.inputFile;
    auto dotPos = baseName.find_last_of('.');
    if (dotPos != std::string::npos) {
        baseName = baseName.substr(0, dotPos);
    }

    std::string cppFilePath = baseName + ".cpp";
    std::string outputBinary = options.outputBinary.empty() ?
        baseName : options.outputBinary;

    // Write the generated C++ file.
    if (!writeFile(cppFilePath, cppCode)) {
        return 1;
    }

    if (options.verbose || options.emitCppOnly) {
        std::cout << "[DictatorScript] Generated C++ written to: " << cppFilePath << std::endl;
    }

    if (options.emitCppOnly) {
        return 0;
    }

    // ---- Stage 5: C++ Compilation ----
    if (options.verbose) {
        std::cout << "[DictatorScript] Stage 5: C++ Compilation..." << std::endl;
    }

    int compileResult = invokeCppCompiler(cppFilePath, outputBinary);
    if (compileResult != 0) {
        return 1;
    }

    if (options.verbose) {
        std::cout << "[DictatorScript] Compilation successful! Output: " << outputBinary << std::endl;
    }

    return 0;
}
