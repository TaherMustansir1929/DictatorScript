/**
 * DictatorScript Linter
 *
 * A lightweight, regex-based linter that catches common errors in .ds files
 * without needing the full compiler. Designed to be resilient to language changes.
 *
 * Rules are modular — each rule is a function that can be individually enabled/disabled.
 */

import * as vscode from 'vscode';
import { ALL_KEYWORDS } from './languageData';

/** Represents a single lint diagnostic. */
interface LintIssue {
    range: vscode.Range;
    message: string;
    severity: vscode.DiagnosticSeverity;
    code?: string;
}

/**
 * A single lint rule.
 * Each rule receives the full document text and line array, and returns issues found.
 */
type LintRule = (doc: vscode.TextDocument, lines: string[]) => LintIssue[];

// ─── Utility Helpers ──────────────────────────────────────────────────

/** Check if a position is inside a comment or string (rough heuristic). */
function isInCommentOrString(line: string, charIndex: number): boolean {
    let inString = false;
    let inChar = false;
    let stringChar = '';
    for (let i = 0; i < charIndex && i < line.length; i++) {
        const c = line[i];
        if (c === '\\' && (inString || inChar)) {
            i++; // skip escaped character
            continue;
        }
        if (!inString && !inChar) {
            if (c === '/' && i + 1 < line.length && line[i + 1] === '/') {
                return true; // rest of line is a comment
            }
            if (c === '"') { inString = true; stringChar = '"'; }
            else if (c === "'") { inChar = true; stringChar = "'"; }
        } else if (inString && c === stringChar) {
            inString = false;
        } else if (inChar && c === stringChar) {
            inChar = false;
        }
    }
    return inString || inChar;
}

/** Strip single-line comments and string contents from a line for safe keyword matching. */
function stripCommentsAndStrings(line: string): string {
    let result = '';
    let inStr = false;
    let inChr = false;
    for (let i = 0; i < line.length; i++) {
        const c = line[i];
        if (c === '\\' && (inStr || inChr)) {
            result += '  '; // placeholder for escaped char
            i++;
            continue;
        }
        if (!inStr && !inChr) {
            if (c === '/' && i + 1 < line.length && line[i + 1] === '/') {
                break; // rest is comment
            }
            if (c === '"') { inStr = true; result += ' '; continue; }
            if (c === "'") { inChr = true; result += ' '; continue; }
            result += c;
        } else if (inStr) {
            if (c === '"') { inStr = false; }
            result += ' '; // blank out string contents
        } else if (inChr) {
            if (c === "'") { inChr = false; }
            result += ' ';
        }
    }
    return result;
}

// ─── Lint Rules ────────────────────────────────────────────────────────

/** Rule: Check for unterminated strings on a single line. */
const ruleUnterminatedString: LintRule = (_doc, lines) => {
    const issues: LintIssue[] = [];
    let inBlockComment = false;

    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];

        // Track block comments
        if (inBlockComment) {
            if (line.includes('*/')) {
                inBlockComment = false;
            }
            continue;
        }

        let inStr = false;
        let inChr = false;

        for (let j = 0; j < line.length; j++) {
            const c = line[j];

            if (c === '/' && j + 1 < line.length) {
                if (line[j + 1] === '/') break; // line comment
                if (line[j + 1] === '*') { inBlockComment = true; break; }
            }

            if (c === '\\' && (inStr || inChr)) { j++; continue; }

            if (!inStr && !inChr) {
                if (c === '"') inStr = true;
                else if (c === "'") inChr = true;
            } else if (inStr && c === '"') {
                inStr = false;
            } else if (inChr && c === "'") {
                inChr = false;
            }
        }

        if (inStr) {
            const col = line.lastIndexOf('"');
            issues.push({
                range: new vscode.Range(i, col, i, line.length),
                message: 'Unterminated string literal.',
                severity: vscode.DiagnosticSeverity.Error,
                code: 'unterminated-string',
            });
        }
        if (inChr) {
            const col = line.lastIndexOf("'");
            issues.push({
                range: new vscode.Range(i, col, i, line.length),
                message: 'Unterminated character literal.',
                severity: vscode.DiagnosticSeverity.Error,
                code: 'unterminated-char',
            });
        }
    }
    return issues;
};

/** Rule: Check for unmatched braces. */
const ruleUnmatchedBraces: LintRule = (_doc, lines) => {
    const issues: LintIssue[] = [];
    const stack: { line: number; col: number; char: string }[] = [];
    const pairs: Record<string, string> = { '{': '}', '(': ')', '[': ']' };
    const closers: Record<string, string> = { '}': '{', ')': '(', ']': '[' };
    let inBlockComment = false;

    for (let i = 0; i < lines.length; i++) {
        const stripped = stripCommentsAndStrings(lines[i]);

        for (let j = 0; j < stripped.length; j++) {
            const c = stripped[j];

            if (c === '/' && j + 1 < stripped.length && stripped[j + 1] === '*') {
                inBlockComment = true;
                j++;
                continue;
            }
            if (inBlockComment) {
                if (c === '*' && j + 1 < stripped.length && stripped[j + 1] === '/') {
                    inBlockComment = false;
                    j++;
                }
                continue;
            }

            if (pairs[c]) {
                stack.push({ line: i, col: j, char: c });
            } else if (closers[c]) {
                if (stack.length === 0 || stack[stack.length - 1].char !== closers[c]) {
                    issues.push({
                        range: new vscode.Range(i, j, i, j + 1),
                        message: `Unmatched '${c}'.`,
                        severity: vscode.DiagnosticSeverity.Error,
                        code: 'unmatched-brace',
                    });
                } else {
                    stack.pop();
                }
            }
        }
    }

    for (const unmatched of stack) {
        issues.push({
            range: new vscode.Range(unmatched.line, unmatched.col, unmatched.line, unmatched.col + 1),
            message: `Unmatched '${unmatched.char}'.`,
            severity: vscode.DiagnosticSeverity.Error,
            code: 'unmatched-brace',
        });
    }

    return issues;
};

/** Rule: Warn about use of C/C++ keywords that are not DictatorScript keywords. */
const ruleCppKeywordMisuse: LintRule = (_doc, lines) => {
    const issues: LintIssue[] = [];
    const cppOnlyKeywords = [
        'if', 'else', 'for', 'while', 'return', 'break', 'continue',
        'class', 'struct', 'const', 'new', 'delete', 'true', 'false',
        'switch', 'case', 'default', 'do', 'try', 'catch', 'throw',
        'public', 'private', 'protected', 'namespace', 'using', 'typedef',
        'cout', 'cin', 'endl', 'printf', 'scanf',
    ];
    // Map of C++ keyword -> DictatorScript equivalent
    const suggestions: Record<string, string> = {
        'if': 'interrogate',
        'else': 'otherwise',
        'for': 'impose',
        'while': 'impose',
        'return': 'report',
        'break': 'silence',
        'continue': 'proceed',
        'struct': 'law',
        'const': 'elite',
        'new': 'summon',
        'delete': 'kill',
        'true': 'loyal',
        'false': 'traitor',
        'cout': 'broadcast',
        'cin': 'demand',
        'class': 'law',
    };

    let inBlockComment = false;
    for (let i = 0; i < lines.length; i++) {
        const raw = lines[i];
        if (inBlockComment) {
            if (raw.includes('*/')) inBlockComment = false;
            continue;
        }
        if (raw.trimStart().startsWith('//')) continue;
        if (raw.includes('/*')) inBlockComment = true;

        const stripped = stripCommentsAndStrings(raw);
        for (const kw of cppOnlyKeywords) {
            const regex = new RegExp(`\\b${kw}\\b`, 'g');
            let match;
            while ((match = regex.exec(stripped)) !== null) {
                // Don't flag if it's actually a DS keyword too (e.g., 'import')
                if (ALL_KEYWORDS.has(kw)) continue;
                const suggestion = suggestions[kw] ? ` Did you mean '${suggestions[kw]}'?` : '';
                issues.push({
                    range: new vscode.Range(i, match.index, i, match.index + kw.length),
                    message: `'${kw}' is not a DictatorScript keyword.${suggestion}`,
                    severity: vscode.DiagnosticSeverity.Warning,
                    code: 'cpp-keyword',
                });
            }
        }
    }
    return issues;
};

/** Rule: Ensure 'regime start' exists in the file (only for non-imported files). */
const ruleRegimeStart: LintRule = (doc, lines) => {
    const issues: LintIssue[] = [];
    const text = doc.getText();
    // Skip files that look like library/import modules (heuristic: contain only law/command, no regime)
    const hasImportableContent = /\b(command|law)\b/.test(text);
    const hasRegime = /\bregime\s+start\b/.test(text);
    const hasImport = /\bimport\b/.test(text);

    // If file has regime, check it's properly formed
    if (hasRegime) {
        for (let i = 0; i < lines.length; i++) {
            const stripped = stripCommentsAndStrings(lines[i]);
            const match = stripped.match(/\bregime\b/);
            if (match) {
                if (!/\bregime\s+start\b/.test(stripped)) {
                    issues.push({
                        range: new vscode.Range(i, match.index || 0, i, (match.index || 0) + 6),
                        message: "'regime' must be followed by 'start'.",
                        severity: vscode.DiagnosticSeverity.Error,
                        code: 'regime-missing-start',
                    });
                }
            }
        }
    } else if (!hasImportableContent && !hasImport && lines.some(l => l.trim().length > 0)) {
        // Only warn for files that don't look like library modules
        issues.push({
            range: new vscode.Range(0, 0, 0, 0),
            message: "No 'regime start' entry point found. Every main program needs one.",
            severity: vscode.DiagnosticSeverity.Information,
            code: 'no-regime',
        });
    }

    return issues;
};

/** Rule: Check for unknown escape sequences in strings. */
const ruleUnknownEscapes: LintRule = (_doc, lines) => {
    const issues: LintIssue[] = [];
    const validEscapes = new Set(['n', 't', '\\', '"', "'", '0']);
    let inBlockComment = false;

    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        if (inBlockComment) {
            if (line.includes('*/')) inBlockComment = false;
            continue;
        }

        let inStr = false;
        let inChr = false;

        for (let j = 0; j < line.length; j++) {
            const c = line[j];
            if (!inStr && !inChr) {
                if (c === '/' && j + 1 < line.length) {
                    if (line[j + 1] === '/') break;
                    if (line[j + 1] === '*') { inBlockComment = true; break; }
                }
                if (c === '"') inStr = true;
                else if (c === "'") inChr = true;
            } else {
                if (c === '\\' && j + 1 < line.length) {
                    const next = line[j + 1];
                    if (!validEscapes.has(next)) {
                        issues.push({
                            range: new vscode.Range(i, j, i, j + 2),
                            message: `Unknown escape sequence '\\${next}'.`,
                            severity: vscode.DiagnosticSeverity.Warning,
                            code: 'unknown-escape',
                        });
                    }
                    j++; // skip next
                    continue;
                }
                if (inStr && c === '"') inStr = false;
                if (inChr && c === "'") inChr = false;
            }
        }
    }
    return issues;
};

/** Rule: Check for bare & or | (should be && or ||). */
const ruleBareLogicalOps: LintRule = (_doc, lines) => {
    const issues: LintIssue[] = [];
    let inBlockComment = false;

    for (let i = 0; i < lines.length; i++) {
        const stripped = stripCommentsAndStrings(lines[i]);
        if (inBlockComment) {
            if (stripped.includes('*/')) inBlockComment = false;
            continue;
        }
        if (stripped.includes('/*')) inBlockComment = true;

        // Match single & not part of && and not the @ operator context
        for (let j = 0; j < stripped.length; j++) {
            const c = stripped[j];
            if (c === '&' && (j + 1 >= stripped.length || stripped[j + 1] !== '&')) {
                if (j > 0 && stripped[j - 1] === '&') continue; // second char of &&
                issues.push({
                    range: new vscode.Range(i, j, i, j + 1),
                    message: "Unexpected '&'. Use '@' for address-of or '&&' for logical AND.",
                    severity: vscode.DiagnosticSeverity.Error,
                    code: 'bare-ampersand',
                });
            }
            if (c === '|' && (j + 1 >= stripped.length || stripped[j + 1] !== '|')) {
                if (j > 0 && stripped[j - 1] === '|') continue;
                issues.push({
                    range: new vscode.Range(i, j, i, j + 1),
                    message: "Unexpected '|'. Use '||' for logical OR.",
                    severity: vscode.DiagnosticSeverity.Error,
                    code: 'bare-pipe',
                });
            }
        }
    }
    return issues;
};

/** Rule: Warn about unterminated block comments. */
const ruleUnterminatedBlockComment: LintRule = (_doc, lines) => {
    const issues: LintIssue[] = [];
    let commentStartLine = -1;
    let commentStartCol = -1;
    let inBlockComment = false;
    let inStr = false;
    let inChr = false;

    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        for (let j = 0; j < line.length; j++) {
            const c = line[j];

            if (!inBlockComment && !inStr && !inChr) {
                if (c === '/' && j + 1 < line.length && line[j + 1] === '/') break;
                if (c === '"') { inStr = true; continue; }
                if (c === "'") { inChr = true; continue; }
                if (c === '/' && j + 1 < line.length && line[j + 1] === '*') {
                    inBlockComment = true;
                    commentStartLine = i;
                    commentStartCol = j;
                    j++;
                    continue;
                }
            } else if (inBlockComment) {
                if (c === '*' && j + 1 < line.length && line[j + 1] === '/') {
                    inBlockComment = false;
                    j++;
                    continue;
                }
            } else if (inStr) {
                if (c === '\\') { j++; continue; }
                if (c === '"') inStr = false;
            } else if (inChr) {
                if (c === '\\') { j++; continue; }
                if (c === "'") inChr = false;
            }
        }
    }

    if (inBlockComment) {
        issues.push({
            range: new vscode.Range(commentStartLine, commentStartCol, commentStartLine, commentStartCol + 2),
            message: 'Unterminated block comment.',
            severity: vscode.DiagnosticSeverity.Error,
            code: 'unterminated-block-comment',
        });
    }

    return issues;
};

// ─── Linter Engine ─────────────────────────────────────────────────────

/** All active lint rules. Add new rules here. */
const ALL_RULES: LintRule[] = [
    ruleUnterminatedString,
    ruleUnterminatedBlockComment,
    ruleUnmatchedBraces,
    ruleCppKeywordMisuse,
    ruleRegimeStart,
    ruleUnknownEscapes,
    ruleBareLogicalOps,
];

/**
 * Run all lint rules on a document and return diagnostics.
 */
export function lintDocument(doc: vscode.TextDocument): vscode.Diagnostic[] {
    const lines = doc.getText().split('\n');
    const allIssues: LintIssue[] = [];

    for (const rule of ALL_RULES) {
        try {
            const issues = rule(doc, lines);
            allIssues.push(...issues);
        } catch {
            // Silently skip rules that throw — resilience over correctness
        }
    }

    return allIssues.map(issue => {
        const diag = new vscode.Diagnostic(issue.range, issue.message, issue.severity);
        diag.source = 'DictatorScript';
        if (issue.code) {
            diag.code = issue.code;
        }
        return diag;
    });
}
