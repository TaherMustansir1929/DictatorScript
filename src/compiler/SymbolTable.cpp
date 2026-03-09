#include "SymbolTable.h"

SymbolTable::SymbolTable() {
    // Start with the global scope.
    scopes_.emplace_back();
}

void SymbolTable::enterScope() {
    scopes_.emplace_back();
}

void SymbolTable::exitScope() {
    if (scopes_.size() > 1) {
        scopes_.pop_back();
    }
}

bool SymbolTable::declare(const SymbolInfo& info) {
    auto& currentScope = scopes_.back();
    if (currentScope.find(info.name) != currentScope.end()) {
        return false; // already declared in this scope
    }
    currentScope[info.name] = info;
    return true;
}

std::optional<SymbolInfo> SymbolTable::lookup(const std::string& name) const {
    // Search from innermost scope to outermost.
    for (int i = (int)scopes_.size() - 1; i >= 0; i--) {
        auto it = scopes_[i].find(name);
        if (it != scopes_[i].end()) {
            return it->second;
        }
    }
    return std::nullopt;
}

std::optional<SymbolInfo> SymbolTable::lookupCurrent(const std::string& name) const {
    auto it = scopes_.back().find(name);
    if (it != scopes_.back().end()) {
        return it->second;
    }
    return std::nullopt;
}

int SymbolTable::depth() const {
    return (int)scopes_.size() - 1;
}
