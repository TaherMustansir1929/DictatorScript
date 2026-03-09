#include "compiler/Compiler.h"
#include <iostream>
#include <string>
#include <vector>

// Print usage information.
void printUsage(const char* programName) {
    std::cout << "DictatorScript Compiler (dsc)\n"
              << "Usage: " << programName << " <source.ds> [options]\n\n"
              << "Options:\n"
              << "  -o <name>       Set output binary name\n"
              << "  --emit-cpp      Only emit .cpp file, don't invoke C++ compiler\n"
              << "  --dump-tokens   Print the token stream and exit\n"
              << "  --dump-ast      Print the AST and exit\n"
              << "  --verbose       Print each pipeline stage\n"
              << "  --help          Show this help message\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    Compiler::Options options;
    std::vector<std::string> args(argv + 1, argv + argc);

    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "--help" || args[i] == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (args[i] == "-o" && i + 1 < args.size()) {
            options.outputBinary = args[++i];
        } else if (args[i] == "--emit-cpp") {
            options.emitCppOnly = true;
        } else if (args[i] == "--dump-tokens") {
            options.dumpTokens = true;
        } else if (args[i] == "--dump-ast") {
            options.dumpAST = true;
        } else if (args[i] == "--verbose") {
            options.verbose = true;
        } else if (args[i][0] == '-') {
            std::cerr << "Unknown option: " << args[i] << std::endl;
            printUsage(argv[0]);
            return 1;
        } else {
            options.inputFile = args[i];
        }
    }

    if (options.inputFile.empty()) {
        std::cerr << "Error: No input file specified." << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    Compiler compiler;
    return compiler.compile(options);
}
