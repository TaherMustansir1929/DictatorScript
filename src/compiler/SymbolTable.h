#ifndef DICTATORSCRIPT_SYMBOL_TABLE_H
#define DICTATORSCRIPT_SYMBOL_TABLE_H

#include "ASTNode.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

// Information stored for each symbol (variable, function, struct).
struct SymbolInfo {
    std::string name;
    TypeSpec type;
    bool isConst = false;
    bool isFunction = false;
    bool isStruct = false;
    int line = 0;
    int col = 0;

    // For functions: parameter types and return type
    std::vector<TypeSpec> paramTypes;
    TypeSpec returnType;

    // For structs: field names and types
    std::vector<StructField> fields;
};

// Scoped symbol table implemented as a stack of maps.
// Each scope is a map from name to SymbolInfo.
// Supports entering/exiting scopes and looking up symbols through the scope chain.
//
// Extension point: to add new symbol kinds, extend SymbolInfo.
class SymbolTable {
public:
    SymbolTable();

    // Push a new scope onto the stack.
    void enterScope();

    // Pop the current scope off the stack.
    void exitScope();

    // Declare a symbol in the current (innermost) scope.
    // Returns false if the symbol is already declared in the current scope.
    bool declare(const SymbolInfo& info);

    // Look up a symbol by name, searching from innermost to outermost scope.
    // Returns nullopt if not found.
    std::optional<SymbolInfo> lookup(const std::string& name) const;

    // Look up a symbol only in the current (innermost) scope.
    std::optional<SymbolInfo> lookupCurrent(const std::string& name) const;

    // Get current scope depth (0 = global).
    int depth() const;

private:
    std::vector<std::unordered_map<std::string, SymbolInfo>> scopes_;
};

#endif // DICTATORSCRIPT_SYMBOL_TABLE_H
