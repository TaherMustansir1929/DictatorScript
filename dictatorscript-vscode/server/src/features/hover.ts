/**
 * DictatorScript Hover Provider
 *
 * Shows contextual hover information for keywords, built-in methods,
 * and user-defined symbols (functions, structs, variables, parameters).
 */

import {
    HoverParams,
    Hover,
    MarkupContent,
    MarkupKind,
} from 'vscode-languageserver/node';
import { DocumentManager } from '../documentManager';
import { KEYWORD_DOCS, BUILTIN_METHODS } from '../../../shared/src/languageData';
import { typeToString, SymbolInfo, Parameter, StructField } from '../../../shared/src/types';
import { TokenType } from '../../../shared/src/tokenType';

// ─── Main Handler ───────────────────────────────────────────────────────

/**
 * Compute hover information for the token at the given cursor position.
 * Returns null if no relevant info can be determined.
 */
export function hoverHandler(
    params: HoverParams,
    docManager: DocumentManager,
): Hover | null {
    const uri = params.textDocument.uri;
    const line = params.position.line;
    const col = params.position.character;

    const token = docManager.getTokenAtPosition(uri, line, col);
    if (!token) return null;

    // ── Keyword hover ──────────────────────────────────────────────

    const keywordDoc = KEYWORD_DOCS[token.lexeme];
    if (keywordDoc !== undefined) {
        return makeHover(keywordDoc, token.line, token.col, token.length);
    }

    // ── Built-in method hover (identifier preceded by dot) ─────────

    if (token.type === TokenType.Identifier) {
        const methodInfo = getBuiltinMethodHover(token.lexeme, uri, line, col, docManager);
        if (methodInfo) {
            return makeHover(methodInfo, token.line, token.col, token.length);
        }
    }

    // ── User symbol hover ──────────────────────────────────────────

    if (token.type === TokenType.Identifier) {
        const symbol = docManager.findSymbolAtPosition(uri, line, col);
        if (symbol) {
            const content = formatSymbolHover(symbol);
            return makeHover(content, token.line, token.col, token.length);
        }
    }

    return null;
}

// ─── Built-in Method Detection ──────────────────────────────────────────

/**
 * Check if the identifier at the cursor position is a built-in method
 * (i.e., preceded by a Dot token). If so, return a formatted markdown string.
 */
function getBuiltinMethodHover(
    lexeme: string,
    uri: string,
    line: number,
    col: number,
    docManager: DocumentManager,
): string | null {
    const methodInfo = BUILTIN_METHODS[lexeme];
    if (!methodInfo) return null;

    // Check that the token is preceded by a Dot token
    const state = docManager.get(uri);
    if (!state) return null;

    // Find the index of this token in the token list
    let tokenIndex = -1;
    for (let i = 0; i < state.tokens.length; i++) {
        const t = state.tokens[i];
        if (t.line === line && col >= t.col && col < t.col + t.length) {
            tokenIndex = i;
            break;
        }
    }

    if (tokenIndex <= 0) return null;

    // Walk backwards skipping whitespace-like tokens to find a Dot
    let prevIndex = tokenIndex - 1;
    while (prevIndex >= 0) {
        const prev = state.tokens[prevIndex];
        if (prev.type === TokenType.Newline) {
            prevIndex--;
            continue;
        }
        if (prev.type === TokenType.Dot) {
            // Confirmed: this is a method call after a dot
            return [
                '```ds',
                methodInfo.signature,
                '```',
                '',
                methodInfo.description,
                '',
                `*Applies to: ${methodInfo.appliesTo}*`,
            ].join('\n');
        }
        break;
    }

    return null;
}

// ─── Symbol Formatting ──────────────────────────────────────────────────

/**
 * Format a user-defined symbol into a markdown hover string.
 */
function formatSymbolHover(sym: SymbolInfo): string {
    if (sym.isFunction) {
        return formatFunctionHover(sym);
    }
    if (sym.isStruct) {
        return formatStructHover(sym);
    }
    if (sym.isParameter) {
        return formatParameterHover(sym);
    }
    return formatVariableHover(sym);
}

function formatFunctionHover(sym: SymbolInfo): string {
    const paramList = sym.params
        .map((p: Parameter) => `${typeToString(p.type)} ${p.name}`)
        .join(', ');
    const retStr = sym.returnType ? typeToString(sym.returnType) : 'void';

    return [
        '```ds',
        `command ${sym.name}(${paramList}) -> ${retStr}`,
        '```',
    ].join('\n');
}

function formatStructHover(sym: SymbolInfo): string {
    const fieldsStr = sym.fields
        .map((f: StructField) => `    ${typeToString(f.type)} ${f.name}`)
        .join('\n');

    if (fieldsStr.length === 0) {
        return [
            '```ds',
            `law ${sym.name} {}`,
            '```',
        ].join('\n');
    }

    return [
        '```ds',
        `law ${sym.name} {`,
        fieldsStr,
        '}',
        '```',
    ].join('\n');
}

function formatVariableHover(sym: SymbolInfo): string {
    const constModifier = sym.isConst ? ' elite' : '';
    return [
        '```ds',
        `declare${constModifier} ${typeToString(sym.type)} ${sym.name}`,
        '```',
    ].join('\n');
}

function formatParameterHover(sym: SymbolInfo): string {
    return [
        '```ds',
        `(parameter) ${typeToString(sym.type)} ${sym.name}`,
        '```',
    ].join('\n');
}

// ─── Hover Construction ─────────────────────────────────────────────────

function makeHover(
    markdownContent: string,
    line: number,
    col: number,
    length: number,
): Hover {
    const contents: MarkupContent = {
        kind: MarkupKind.Markdown,
        value: markdownContent,
    };

    return {
        contents,
        range: {
            start: { line, character: col },
            end: { line, character: col + length },
        },
    };
}
