/**
 * DictatorScript Symbol Table
 *
 * Scoped symbol table using a stack of maps.
 * Port of src/compiler/SymbolTable.h/.cpp.
 */

import { SymbolInfo } from '../../shared/src/types';

export class SymbolTable {
    private scopes: Map<string, SymbolInfo>[] = [new Map()];

    enterScope(): void {
        this.scopes.push(new Map());
    }

    exitScope(): void {
        if (this.scopes.length > 1) {
            this.scopes.pop();
        }
    }

    /** Declare a symbol in the current (innermost) scope. Returns false if already exists in current scope. */
    declare(info: SymbolInfo): boolean {
        const current = this.scopes[this.scopes.length - 1];
        if (current.has(info.name)) return false;
        current.set(info.name, info);
        return true;
    }

    /** Look up a symbol from innermost to outermost scope. */
    lookup(name: string): SymbolInfo | undefined {
        for (let i = this.scopes.length - 1; i >= 0; i--) {
            const sym = this.scopes[i].get(name);
            if (sym) return sym;
        }
        return undefined;
    }

    /** Look up a symbol in the current scope only. */
    lookupCurrent(name: string): SymbolInfo | undefined {
        return this.scopes[this.scopes.length - 1].get(name);
    }

    /** Current scope depth (0 = global). */
    depth(): number {
        return this.scopes.length - 1;
    }

    /** Get all symbols visible at the current scope (innermost shadows outer). */
    allVisibleSymbols(): SymbolInfo[] {
        const seen = new Set<string>();
        const result: SymbolInfo[] = [];
        for (let i = this.scopes.length - 1; i >= 0; i--) {
            for (const [name, info] of this.scopes[i]) {
                if (!seen.has(name)) {
                    seen.add(name);
                    result.push(info);
                }
            }
        }
        return result;
    }

    /** Get all symbols in the global scope. */
    globalSymbols(): SymbolInfo[] {
        return Array.from(this.scopes[0].values());
    }
}
