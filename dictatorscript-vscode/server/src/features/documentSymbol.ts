/**
 * DictatorScript Document Symbol Provider
 *
 * Provides hierarchical document symbols for the Outline view.
 * Structs include their fields as children, functions include their
 * parameters as children, and the regime entry point appears as a module.
 */

import {
    DocumentSymbolParams,
    DocumentSymbol,
    SymbolKind,
    Range,
    Position,
} from 'vscode-languageserver/node';
import { DocumentManager } from '../documentManager';
import { typeToString, ScopeInfo, SymbolInfo } from '../../../shared/src/types';
import { TokenType } from '../../../shared/src/tokenType';

/**
 * Handle a textDocument/documentSymbol request.
 *
 * Walks the parse result and builds a hierarchical DocumentSymbol[] for
 * top-level structs, functions, global variables, and the regime entry point.
 */
export function documentSymbolHandler(
    params: DocumentSymbolParams,
    docManager: DocumentManager
): DocumentSymbol[] {
    const uri = params.textDocument.uri;
    const state = docManager.get(uri);
    if (!state) return [];

    const { parseResult, tokens } = state;
    const result: DocumentSymbol[] = [];

    // Process top-level symbols
    for (const sym of parseResult.symbols) {
        if (sym.isStruct && sym.scopeDepth === 0) {
            result.push(buildStructSymbol(sym, parseResult.scopes));
        } else if (sym.isFunction && sym.scopeDepth === 0) {
            result.push(buildFunctionSymbol(sym, parseResult.scopes));
        } else if (sym.scopeDepth === 0 && !sym.isParameter) {
            result.push(buildVariableSymbol(sym));
        }
    }

    // Check for regime start by scanning tokens
    const regimeSymbol = buildRegimeSymbol(tokens, parseResult.scopes);
    if (regimeSymbol) {
        result.push(regimeSymbol);
    }

    return result;
}

/**
 * Build a DocumentSymbol for a struct declaration, with fields as children.
 */
function buildStructSymbol(sym: SymbolInfo, scopes: ScopeInfo[]): DocumentSymbol {
    const selectionRange = Range.create(
        Position.create(sym.line, sym.col),
        Position.create(sym.endLine, sym.endCol)
    );

    // Try to find the scope that covers this struct's body
    const scope = findDeclScope(sym, scopes);
    const range = scope
        ? Range.create(
            Position.create(sym.line, 0),
            Position.create(scope.endLine, scope.endCol)
        )
        : selectionRange;

    // Map struct fields to child DocumentSymbols
    const children: DocumentSymbol[] = sym.fields.map(field => {
        const fieldRange = Range.create(
            Position.create(field.line, field.col),
            Position.create(field.line, field.col + field.name.length)
        );
        return DocumentSymbol.create(
            field.name,
            typeToString(field.type),
            SymbolKind.Field,
            fieldRange,
            fieldRange
        );
    });

    return DocumentSymbol.create(
        sym.name,
        'struct',
        SymbolKind.Struct,
        range,
        selectionRange,
        children
    );
}

/**
 * Build a DocumentSymbol for a function declaration, with parameters as children.
 */
function buildFunctionSymbol(sym: SymbolInfo, scopes: ScopeInfo[]): DocumentSymbol {
    const selectionRange = Range.create(
        Position.create(sym.line, sym.col),
        Position.create(sym.endLine, sym.endCol)
    );

    // Try to find the scope that covers this function's body
    const scope = findDeclScope(sym, scopes);
    const range = scope
        ? Range.create(
            Position.create(sym.line, 0),
            Position.create(scope.endLine, scope.endCol)
        )
        : selectionRange;

    // Map parameters to child DocumentSymbols
    const children: DocumentSymbol[] = sym.params.map(param => {
        const paramRange = Range.create(
            Position.create(param.line, param.col),
            Position.create(param.line, param.col + param.name.length)
        );
        return DocumentSymbol.create(
            param.name,
            typeToString(param.type),
            SymbolKind.Variable,
            paramRange,
            paramRange
        );
    });

    const detail = sym.returnType ? `-> ${typeToString(sym.returnType)}` : '';

    return DocumentSymbol.create(
        sym.name,
        detail,
        SymbolKind.Function,
        range,
        selectionRange,
        children
    );
}

/**
 * Build a DocumentSymbol for a top-level variable.
 */
function buildVariableSymbol(sym: SymbolInfo): DocumentSymbol {
    const range = Range.create(
        Position.create(sym.line, sym.col),
        Position.create(sym.endLine, sym.endCol)
    );

    return DocumentSymbol.create(
        sym.name,
        typeToString(sym.type),
        SymbolKind.Variable,
        range,
        range
    );
}

/**
 * Scan tokens for a `regime start` declaration and build a Module symbol for it.
 */
function buildRegimeSymbol(
    tokens: import('../../../shared/src/token').Token[],
    scopes: ScopeInfo[]
): DocumentSymbol | null {
    for (let i = 0; i < tokens.length; i++) {
        if (tokens[i].type !== TokenType.KW_Regime) continue;

        // Skip past any newlines to find the "start" identifier
        let j = i + 1;
        while (j < tokens.length && tokens[j].type === TokenType.Newline) j++;

        if (j >= tokens.length) continue;
        if (tokens[j].type !== TokenType.Identifier || tokens[j].lexeme !== 'start') continue;

        const regimeToken = tokens[i];
        const startToken = tokens[j];

        // Determine the end of the regime block
        let endLine = startToken.line;
        let endCol = startToken.col + startToken.length;

        // Look for a '{' after "start" to find the body scope
        let k = j + 1;
        while (k < tokens.length && tokens[k].type === TokenType.Newline) k++;

        if (k < tokens.length && tokens[k].type === TokenType.LBrace) {
            const braceToken = tokens[k];
            for (const scope of scopes) {
                if (scope.startLine === braceToken.line && scope.startCol === braceToken.col) {
                    endLine = scope.endLine;
                    endCol = scope.endCol;
                    break;
                }
            }
        }

        const range = Range.create(
            Position.create(regimeToken.line, regimeToken.col),
            Position.create(endLine, endCol)
        );
        const selectionRange = Range.create(
            Position.create(regimeToken.line, regimeToken.col),
            Position.create(startToken.line, startToken.col + startToken.length)
        );

        return DocumentSymbol.create(
            'regime start',
            '',
            SymbolKind.Module,
            range,
            selectionRange
        );
    }

    return null;
}

/**
 * Find the scope that corresponds to a top-level symbol's body.
 *
 * Looks for a direct child of the global scope (parentIndex === 0) whose
 * opening delimiter is on the same line as the symbol name and positioned
 * after the name token.
 */
function findDeclScope(
    sym: { line: number; endCol: number },
    scopes: ScopeInfo[]
): ScopeInfo | undefined {
    let best: ScopeInfo | undefined;
    for (const scope of scopes) {
        if (scope.parentIndex !== 0) continue;
        if (scope.startLine === sym.line && scope.startCol >= sym.endCol) {
            if (!best || scope.startCol < best.startCol) {
                best = scope;
            }
        }
    }
    return best;
}
