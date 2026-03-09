/**
 * DictatorScript Semantic Tokens Provider
 *
 * Provides semantic token highlighting for identifiers based on
 * symbol analysis from the parser. Non-identifier tokens are left
 * to the TextMate grammar.
 */

import { SemanticTokensParams, SemanticTokens } from 'vscode-languageserver/node';
import { DocumentManager } from '../documentManager';
import { TokenType } from '../../../shared/src/tokenType';
import { BUILTIN_METHODS } from '../../../shared/src/languageData';
import { SymbolInfo } from '../../../shared/src/types';

/** The semantic token type legend entries. */
export const tokenTypes: string[] = [
    'type',
    'struct',
    'parameter',
    'variable',
    'property',
    'function',
    'method',
    'keyword',
    'string',
    'number',
    'operator',
];

/** The semantic token modifier legend entries. */
export const tokenModifiers: string[] = [
    'declaration',
    'definition',
    'readonly',
    'defaultLibrary',
];

// Pre-computed index maps for fast lookup
const tokenTypeIndex = new Map<string, number>();
tokenTypes.forEach((t, i) => tokenTypeIndex.set(t, i));

const tokenModifierIndex = new Map<string, number>();
tokenModifiers.forEach((m, i) => tokenModifierIndex.set(m, i));

/** Compute the bitmask for a set of modifier names. */
function modifierBitmask(modifiers: string[]): number {
    let mask = 0;
    for (const mod of modifiers) {
        const idx = tokenModifierIndex.get(mod);
        if (idx !== undefined) {
            mask |= 1 << idx;
        }
    }
    return mask;
}

/** A single resolved semantic token before delta-encoding. */
interface ResolvedToken {
    line: number;
    col: number;
    length: number;
    typeIndex: number;
    modifierMask: number;
}

/**
 * Handle a semantic tokens full request.
 */
export function semanticTokensHandler(
    params: SemanticTokensParams,
    docManager: DocumentManager,
): SemanticTokens {
    const state = docManager.get(params.textDocument.uri);
    if (!state) {
        return { data: new Uint32Array(0) } as unknown as SemanticTokens;
    }

    const { tokens, parseResult } = state;

    // Build a map from "line:col" -> SymbolInfo for fast lookup at token positions
    const symbolMap = new Map<string, SymbolInfo>();
    for (const sym of parseResult.symbols) {
        symbolMap.set(`${sym.line}:${sym.col}`, sym);
    }

    // Build a set of known struct type names
    const structNames = new Set<string>();
    for (const sym of parseResult.symbols) {
        if (sym.isStruct) {
            structNames.add(sym.name);
        }
    }

    // Build a map from "line:col" -> reference for quick ref lookup
    const refMap = new Map<string, { isDefinition: boolean; isMemberAccess: boolean }>();
    for (const ref of parseResult.references) {
        refMap.set(`${ref.line}:${ref.col}`, {
            isDefinition: ref.isDefinition,
            isMemberAccess: ref.isMemberAccess,
        });
    }

    // Build a name -> SymbolInfo map for identifier resolution by name
    // (uses the first matching symbol; scoping is approximate here)
    const symbolByName = new Map<string, SymbolInfo>();
    for (const sym of parseResult.symbols) {
        if (!symbolByName.has(sym.name)) {
            symbolByName.set(sym.name, sym);
        }
    }

    const resolved: ResolvedToken[] = [];

    for (let i = 0; i < tokens.length; i++) {
        const token = tokens[i];

        if (token.type !== TokenType.Identifier) {
            // Skip non-identifier tokens; TextMate grammar handles those
            continue;
        }

        const key = `${token.line}:${token.col}`;
        const ref = refMap.get(key);
        const symAtPos = symbolMap.get(key);
        const symByName = symbolByName.get(token.lexeme);

        // Determine the effective symbol info
        const sym = symAtPos || symByName;

        // Check if preceded by a Dot token
        const prevToken = findPrevNonTrivialToken(tokens, i);
        const isPrecededByDot = prevToken !== undefined && prevToken.type === TokenType.Dot;

        // Check if followed by LParen (function/method call)
        const nextToken = findNextNonTrivialToken(tokens, i);
        const isFollowedByLParen = nextToken !== undefined && nextToken.type === TokenType.LParen;

        if (isPrecededByDot) {
            // Member access: property or method
            if (isFollowedByLParen) {
                resolved.push({
                    line: token.line,
                    col: token.col,
                    length: token.length,
                    typeIndex: tokenTypeIndex.get('method')!,
                    modifierMask: BUILTIN_METHODS[token.lexeme]
                        ? modifierBitmask(['defaultLibrary'])
                        : 0,
                });
            } else {
                resolved.push({
                    line: token.line,
                    col: token.col,
                    length: token.length,
                    typeIndex: tokenTypeIndex.get('property')!,
                    modifierMask: 0,
                });
            }
            continue;
        }

        // Check if this identifier is a struct type name used in a type position
        if (structNames.has(token.lexeme)) {
            // If this is the definition site of the struct itself, mark as 'type'
            if (sym && sym.isStruct && ref && ref.isDefinition) {
                resolved.push({
                    line: token.line,
                    col: token.col,
                    length: token.length,
                    typeIndex: tokenTypeIndex.get('type')!,
                    modifierMask: modifierBitmask(['declaration', 'definition']),
                });
                continue;
            }

            // If it's a reference to a struct name (used as a type), mark as 'type'
            // Heuristic: preceded by 'declare', a type keyword context, or is in a
            // type position (followed by an identifier = being used as type annotation)
            if (isInTypePosition(tokens, i) || (ref && !ref.isMemberAccess)) {
                resolved.push({
                    line: token.line,
                    col: token.col,
                    length: token.length,
                    typeIndex: tokenTypeIndex.get('type')!,
                    modifierMask: 0,
                });
                continue;
            }
        }

        if (!sym) {
            // Unknown identifier — no semantic info
            continue;
        }

        if (sym.isFunction) {
            const mods: string[] = [];
            if (ref && ref.isDefinition) {
                mods.push('declaration');
            }
            resolved.push({
                line: token.line,
                col: token.col,
                length: token.length,
                typeIndex: tokenTypeIndex.get('function')!,
                modifierMask: modifierBitmask(mods),
            });
            continue;
        }

        if (sym.isParameter) {
            resolved.push({
                line: token.line,
                col: token.col,
                length: token.length,
                typeIndex: tokenTypeIndex.get('parameter')!,
                modifierMask: ref && ref.isDefinition
                    ? modifierBitmask(['declaration'])
                    : 0,
            });
            continue;
        }

        if (sym.isConst) {
            resolved.push({
                line: token.line,
                col: token.col,
                length: token.length,
                typeIndex: tokenTypeIndex.get('variable')!,
                modifierMask: modifierBitmask(['readonly']),
            });
            continue;
        }

        // Regular variable
        resolved.push({
            line: token.line,
            col: token.col,
            length: token.length,
            typeIndex: tokenTypeIndex.get('variable')!,
            modifierMask: ref && ref.isDefinition
                ? modifierBitmask(['declaration'])
                : 0,
        });
    }

    // Sort tokens by position (line, then col) for correct delta encoding
    resolved.sort((a, b) => a.line !== b.line ? a.line - b.line : a.col - b.col);

    // Encode as delta array per the LSP semantic tokens protocol:
    // Each token is 5 integers: deltaLine, deltaStart, length, tokenType, tokenModifiers
    const data: number[] = [];
    let prevLine = 0;
    let prevCol = 0;

    for (const tok of resolved) {
        const deltaLine = tok.line - prevLine;
        const deltaStart = deltaLine === 0 ? tok.col - prevCol : tok.col;

        data.push(deltaLine, deltaStart, tok.length, tok.typeIndex, tok.modifierMask);

        prevLine = tok.line;
        prevCol = tok.col;
    }

    return { data: new Uint32Array(data) } as unknown as SemanticTokens;
}

// ─── Helpers ──────────────────────────────────────────────────────────

/**
 * Find the previous token that is not Newline, Semicolon, or EOF.
 */
function findPrevNonTrivialToken(
    tokens: import('../../../shared/src/token').Token[],
    index: number,
): import('../../../shared/src/token').Token | undefined {
    for (let i = index - 1; i >= 0; i--) {
        const t = tokens[i];
        if (t.type !== TokenType.Newline && t.type !== TokenType.Semicolon && t.type !== TokenType.EOF) {
            return t;
        }
    }
    return undefined;
}

/**
 * Find the next token that is not Newline, Semicolon, or EOF.
 */
function findNextNonTrivialToken(
    tokens: import('../../../shared/src/token').Token[],
    index: number,
): import('../../../shared/src/token').Token | undefined {
    for (let i = index + 1; i < tokens.length; i++) {
        const t = tokens[i];
        if (t.type !== TokenType.Newline && t.type !== TokenType.Semicolon && t.type !== TokenType.EOF) {
            return t;
        }
    }
    return undefined;
}

/**
 * Heuristic: check if an identifier at position `index` is in a type position.
 *
 * Returns true if:
 * - Preceded by 'declare', 'elite', or a type keyword (e.g., in "declare StructName varName")
 * - Followed by an identifier (the variable name), suggesting this is a type annotation
 * - Preceded by 'law' keyword (struct definition)
 */
function isInTypePosition(
    tokens: import('../../../shared/src/token').Token[],
    index: number,
): boolean {
    // Check if preceded by 'declare', 'elite', or 'law'
    for (let i = index - 1; i >= 0; i--) {
        const t = tokens[i];
        if (t.type === TokenType.Newline || t.type === TokenType.Semicolon) break;
        if (t.type === TokenType.KW_Declare || t.type === TokenType.KW_Elite || t.type === TokenType.KW_Law) {
            return true;
        }
        // If preceded by a comma within a parameter list, also counts as type position
        if (t.type === TokenType.Comma || t.type === TokenType.LParen) {
            // Could be a function parameter type
            return true;
        }
    }

    // Check if followed by an identifier (suggesting "Type VarName" pattern)
    const next = findNextNonTrivialToken(tokens, index);
    if (next && next.type === TokenType.Identifier) {
        return true;
    }

    // Check if followed by [] (array type) or -> (pointer type)
    if (next && (next.type === TokenType.LBracket || next.type === TokenType.Arrow)) {
        return true;
    }

    return false;
}
