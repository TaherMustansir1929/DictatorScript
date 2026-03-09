/**
 * DictatorScript Diagnostics Provider
 *
 * Merges lexer diagnostics, parser diagnostics, and unmatched-brace checks
 * into a single array of LSP Diagnostic objects.
 */

import { Diagnostic, DiagnosticSeverity, Range, Position } from 'vscode-languageserver/node';
import { DocumentState } from '../documentManager';
import { Token } from '../../../shared/src/token';
import { TokenType } from '../../../shared/src/tokenType';

// ─── Helpers ────────────────────────────────────────────────────────────

/** Map severity strings from internal diagnostics to LSP DiagnosticSeverity. */
function mapSeverity(sev: 'error' | 'warning' | 'info'): DiagnosticSeverity {
    switch (sev) {
        case 'error':   return DiagnosticSeverity.Error;
        case 'warning': return DiagnosticSeverity.Warning;
        case 'info':    return DiagnosticSeverity.Information;
    }
}

/** Opening bracket types and their matching closers. */
const OPEN_BRACKETS: ReadonlyMap<TokenType, TokenType> = new Map([
    [TokenType.LBrace,   TokenType.RBrace],
    [TokenType.LParen,   TokenType.RParen],
    [TokenType.LBracket, TokenType.RBracket],
]);

/** Closing bracket types mapped back to their openers. */
const CLOSE_BRACKETS: ReadonlyMap<TokenType, TokenType> = new Map([
    [TokenType.RBrace,   TokenType.LBrace],
    [TokenType.RParen,   TokenType.LParen],
    [TokenType.RBracket, TokenType.LBracket],
]);

/** Human-readable labels for bracket types. */
function bracketLabel(type: TokenType): string {
    switch (type) {
        case TokenType.LBrace:   return '{';
        case TokenType.RBrace:   return '}';
        case TokenType.LParen:   return '(';
        case TokenType.RParen:   return ')';
        case TokenType.LBracket: return '[';
        case TokenType.RBracket: return ']';
        default:                 return '?';
    }
}

function matchingBracketLabel(type: TokenType): string {
    switch (type) {
        case TokenType.LBrace:   return '}';
        case TokenType.RBrace:   return '{';
        case TokenType.LParen:   return ')';
        case TokenType.RParen:   return '(';
        case TokenType.LBracket: return ']';
        case TokenType.RBracket: return '[';
        default:                 return '?';
    }
}

// ─── Main Function ──────────────────────────────────────────────────────

/**
 * Compute all diagnostics for a document by merging lexer diagnostics,
 * parser diagnostics, and an unmatched-brace check.
 */
export function computeDiagnostics(state: DocumentState): Diagnostic[] {
    const diagnostics: Diagnostic[] = [];

    // 1. Lexer diagnostics
    for (const ld of state.lexerDiagnostics) {
        diagnostics.push({
            range: Range.create(
                Position.create(ld.line, ld.col),
                Position.create(ld.line, ld.endCol),
            ),
            severity: mapSeverity(ld.severity),
            message: ld.message,
            source: 'DictatorScript',
        });
    }

    // 2. Parser diagnostics
    for (const pd of state.parseResult.diagnostics) {
        const diag: Diagnostic = {
            range: Range.create(
                Position.create(pd.line, pd.col),
                Position.create(pd.endLine, pd.endCol),
            ),
            severity: mapSeverity(pd.severity),
            message: pd.message,
            source: 'DictatorScript',
        };
        if (pd.code !== undefined) {
            diag.code = pd.code;
        }
        if (pd.data !== undefined) {
            diag.data = pd.data;
        }
        diagnostics.push(diag);
    }

    // 3. Unmatched brace check
    const braceDiags = checkUnmatchedBraces(state.tokens);
    diagnostics.push(...braceDiags);

    return diagnostics;
}

// ─── Unmatched Brace Checker ────────────────────────────────────────────

interface BraceEntry {
    token: Token;
    expectedClose: TokenType;
}

function checkUnmatchedBraces(tokens: Token[]): Diagnostic[] {
    const diagnostics: Diagnostic[] = [];
    const stack: BraceEntry[] = [];

    for (const token of tokens) {
        // Check if this is an opening bracket
        const expectedClose = OPEN_BRACKETS.get(token.type);
        if (expectedClose !== undefined) {
            stack.push({ token, expectedClose });
            continue;
        }

        // Check if this is a closing bracket
        const expectedOpen = CLOSE_BRACKETS.get(token.type);
        if (expectedOpen !== undefined) {
            if (stack.length > 0 && stack[stack.length - 1].expectedClose === token.type) {
                // Matched: pop the opener
                stack.pop();
            } else {
                // Unmatched closer
                diagnostics.push({
                    range: Range.create(
                        Position.create(token.line, token.col),
                        Position.create(token.line, token.col + token.length),
                    ),
                    severity: DiagnosticSeverity.Error,
                    message: `Unmatched '${bracketLabel(token.type)}' — no matching '${matchingBracketLabel(token.type)}' found.`,
                    source: 'DictatorScript',
                    code: 'unmatched-brace',
                });
            }
        }
    }

    // Any remaining openers on the stack are unmatched
    for (const entry of stack) {
        diagnostics.push({
            range: Range.create(
                Position.create(entry.token.line, entry.token.col),
                Position.create(entry.token.line, entry.token.col + entry.token.length),
            ),
            severity: DiagnosticSeverity.Error,
            message: `Unmatched '${bracketLabel(entry.token.type)}' — no matching '${matchingBracketLabel(entry.token.type)}' found.`,
            source: 'DictatorScript',
            code: 'unmatched-brace',
        });
    }

    return diagnostics;
}
