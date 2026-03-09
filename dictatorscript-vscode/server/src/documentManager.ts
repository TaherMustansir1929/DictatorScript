/**
 * DictatorScript Document Manager
 *
 * Manages per-document parse state. Re-lexes and re-parses on every change.
 */

import { TextDocument } from 'vscode-languageserver-textdocument';
import { Token } from '../../shared/src/token';
import { TokenType } from '../../shared/src/tokenType';
import { ParseResult, SymbolInfo, ScopeInfo, LexerDiagnostic } from '../../shared/src/types';
import { tokenize, LexResult } from './lexer';
import { parse } from './parser';

export interface DocumentState {
    document: TextDocument;
    tokens: Token[];
    parseResult: ParseResult;
    lexerDiagnostics: LexerDiagnostic[];
}

export class DocumentManager {
    private states = new Map<string, DocumentState>();

    update(document: TextDocument): DocumentState {
        const lexResult: LexResult = tokenize(document.getText());
        const parseResult = parse(lexResult.tokens);

        const state: DocumentState = {
            document,
            tokens: lexResult.tokens,
            parseResult,
            lexerDiagnostics: lexResult.diagnostics,
        };

        this.states.set(document.uri, state);
        return state;
    }

    get(uri: string): DocumentState | undefined {
        return this.states.get(uri);
    }

    remove(uri: string): void {
        this.states.delete(uri);
    }

    /** Find the token at a given position. */
    getTokenAtPosition(uri: string, line: number, col: number): Token | undefined {
        const state = this.states.get(uri);
        if (!state) return undefined;

        for (const token of state.tokens) {
            if (token.type === TokenType.Newline || token.type === TokenType.EOF) continue;
            if (token.line === line && col >= token.col && col < token.col + token.length) {
                return token;
            }
        }
        return undefined;
    }

    /** Find the innermost scope containing the given position. */
    getScopeAtPosition(uri: string, line: number, col: number): ScopeInfo | undefined {
        const state = this.states.get(uri);
        if (!state) return undefined;

        let best: ScopeInfo | undefined;
        for (const scope of state.parseResult.scopes) {
            if (this.positionInScope(line, col, scope)) {
                if (!best || this.scopeContains(best, scope)) {
                    // scope is more specific (nested inside best)
                    best = scope;
                } else if (!this.scopeContains(scope, best)) {
                    best = scope;
                }
            }
        }
        return best;
    }

    /** Get all symbols visible at a given position by walking the scope tree. */
    getVisibleSymbolsAt(uri: string, line: number, col: number): SymbolInfo[] {
        const state = this.states.get(uri);
        if (!state) return [];

        const seen = new Set<string>();
        const result: SymbolInfo[] = [];

        // Find the scope chain from innermost to outermost
        const scope = this.getScopeAtPosition(uri, line, col);
        if (!scope) {
            // Fall back to global scope
            for (const sym of state.parseResult.symbols) {
                if (sym.scopeDepth === 0 && !seen.has(sym.name)) {
                    seen.add(sym.name);
                    result.push(sym);
                }
            }
            return result;
        }

        // Walk from innermost scope to global, collecting symbols
        let current: ScopeInfo | undefined = scope;
        while (current) {
            for (const sym of current.symbols) {
                if (!seen.has(sym.name)) {
                    // Only include symbols declared before the cursor position
                    if (sym.line < line || (sym.line === line && sym.col <= col)) {
                        seen.add(sym.name);
                        result.push(sym);
                    }
                }
            }
            current = current.parentIndex >= 0 ? state.parseResult.scopes[current.parentIndex] : undefined;
        }

        return result;
    }

    /** Find the symbol definition for an identifier at a given position. */
    findSymbolAtPosition(uri: string, line: number, col: number): SymbolInfo | undefined {
        const token = this.getTokenAtPosition(uri, line, col);
        if (!token || token.type !== TokenType.Identifier) return undefined;

        const visibleSymbols = this.getVisibleSymbolsAt(uri, line, col);
        return visibleSymbols.find(s => s.name === token.lexeme);
    }

    // ─── Private Helpers ────────────────────────────────────────────

    private positionInScope(line: number, col: number, scope: ScopeInfo): boolean {
        if (line < scope.startLine) return false;
        if (line > scope.endLine) return false;
        if (line === scope.startLine && col < scope.startCol) return false;
        if (line === scope.endLine && col > scope.endCol) return false;
        return true;
    }

    /** Check if outer contains inner (inner is more specific). */
    private scopeContains(outer: ScopeInfo, inner: ScopeInfo): boolean {
        if (inner.startLine < outer.startLine) return false;
        if (inner.endLine > outer.endLine) return false;
        if (inner.startLine === outer.startLine && inner.startCol < outer.startCol) return false;
        if (inner.endLine === outer.endLine && inner.endCol > outer.endCol) return false;
        return true;
    }
}
