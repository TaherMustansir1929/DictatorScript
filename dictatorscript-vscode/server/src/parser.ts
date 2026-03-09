/**
 * DictatorScript Parser — Lightweight Symbol Extractor
 *
 * Error-tolerant parser that extracts declarations, references, and scopes
 * from a token stream. Designed for LSP use — runs on every keystroke
 * and tolerates incomplete/broken code.
 */

import { Token } from '../../shared/src/token';
import { TokenType, isTypeKeyword } from '../../shared/src/tokenType';
import {
    SymbolInfo, SymbolReference, ScopeInfo, ParseResult,
    ParserDiagnostic, TypeSpec, TypeKind, StructField, Parameter,
    makePrimitive, makeArray, makePointer, makeMap, makeStruct,
} from '../../shared/src/types';
import {
    ALL_KEYWORDS, CPP_ONLY_KEYWORDS, CPP_KEYWORD_SUGGESTIONS,
} from '../../shared/src/languageData';
import { SymbolTable } from './symbolTable';

export function parse(tokens: Token[]): ParseResult {
    const parser = new Parser(tokens);
    return parser.parse();
}

class Parser {
    private tokens: Token[];
    private pos = 0;
    private symbols: SymbolInfo[] = [];
    private references: SymbolReference[] = [];
    private scopes: ScopeInfo[] = [];
    private diagnostics: ParserDiagnostic[] = [];
    private importPaths: string[] = [];
    private symTable = new SymbolTable();
    private loopDepth = 0;
    private scopeStack: number[] = []; // stack of indices into this.scopes
    private hasRegime = false;

    constructor(tokens: Token[]) {
        this.tokens = tokens;
    }

    parse(): ParseResult {
        // Create global scope
        this.scopes.push({
            startLine: 0, startCol: 0,
            endLine: Infinity, endCol: Infinity,
            parentIndex: -1, symbols: [],
        });
        this.scopeStack.push(0);

        // Pass 1: Pre-register top-level structs and functions for forward references
        this.preRegisterTopLevel();

        // Pass 2: Full parse
        this.pos = 0;
        this.skipNewlines();
        while (!this.isAtEnd()) {
            this.parseTopLevel();
            this.skipNewlines();
        }

        // Check for missing regime
        if (!this.hasRegime) {
            const hasContent = this.symbols.some(s => s.isFunction || s.isStruct);
            if (!hasContent && this.tokens.length > 1) {
                this.diagnostics.push({
                    line: 0, col: 0, endLine: 0, endCol: 0,
                    message: "No 'regime start' entry point found. Every main program needs one.",
                    severity: 'info',
                    code: 'no-regime',
                });
            }
        }

        // Close global scope
        const lastToken = this.tokens[this.tokens.length - 1];
        this.scopes[0].endLine = lastToken.line;
        this.scopes[0].endCol = lastToken.col;

        return {
            symbols: this.symbols,
            references: this.references,
            scopes: this.scopes,
            diagnostics: this.diagnostics,
            importPaths: this.importPaths,
        };
    }

    // ─── Pre-registration (Pass 1) ─────────────────────────────────

    private preRegisterTopLevel(): void {
        let i = 0;
        while (i < this.tokens.length) {
            const t = this.tokens[i];

            if (t.type === TokenType.KW_Law && i + 1 < this.tokens.length) {
                const nameToken = this.tokens[i + 1];
                if (nameToken.type === TokenType.Identifier) {
                    const sym: SymbolInfo = {
                        name: nameToken.lexeme, type: makeStruct(nameToken.lexeme),
                        isConst: false, isFunction: false, isStruct: true, isParameter: false,
                        line: nameToken.line, col: nameToken.col,
                        endLine: nameToken.line, endCol: nameToken.col + nameToken.length,
                        params: [], fields: [], scopeDepth: 0,
                    };
                    this.symTable.declare(sym);
                    this.symbols.push(sym);
                    this.scopes[0].symbols.push(sym);
                    this.references.push({
                        name: nameToken.lexeme, line: nameToken.line, col: nameToken.col,
                        length: nameToken.length, isDefinition: true, isMemberAccess: false,
                    });
                }
            } else if (t.type === TokenType.KW_Command && i + 1 < this.tokens.length) {
                const nameToken = this.tokens[i + 1];
                if (nameToken.type === TokenType.Identifier) {
                    const sym: SymbolInfo = {
                        name: nameToken.lexeme, type: makePrimitive('function'),
                        isConst: false, isFunction: true, isStruct: false, isParameter: false,
                        line: nameToken.line, col: nameToken.col,
                        endLine: nameToken.line, endCol: nameToken.col + nameToken.length,
                        params: [], fields: [], scopeDepth: 0,
                    };
                    this.symTable.declare(sym);
                    this.symbols.push(sym);
                    this.scopes[0].symbols.push(sym);
                    this.references.push({
                        name: nameToken.lexeme, line: nameToken.line, col: nameToken.col,
                        length: nameToken.length, isDefinition: true, isMemberAccess: false,
                    });
                }
            }
            i++;
        }
    }

    // ─── Top-Level Parsing ──────────────────────────────────────────

    private parseTopLevel(): void {
        const t = this.current();
        if (!t) return;

        switch (t.type) {
            case TokenType.KW_Law:
                this.parseStructDef();
                break;
            case TokenType.KW_Command:
                this.parseFunctionDef();
                break;
            case TokenType.KW_Regime:
                this.parseRegime();
                break;
            case TokenType.KW_Import:
                this.parseImport();
                break;
            default:
                this.diagnostics.push({
                    line: t.line, col: t.col,
                    endLine: t.line, endCol: t.col + t.length,
                    message: "Expected top-level declaration (law, command, regime, or import).",
                    severity: 'error',
                });
                this.synchronize();
                break;
        }
    }

    // ─── Struct Definition ──────────────────────────────────────────

    private parseStructDef(): void {
        this.advance(); // skip 'law'
        this.skipNewlines();

        if (!this.check(TokenType.Identifier)) {
            this.synchronize();
            return;
        }

        const nameToken = this.current()!;
        const existingSym = this.symTable.lookup(nameToken.lexeme);
        this.advance();
        this.skipNewlines();

        if (!this.match(TokenType.LBrace)) {
            this.synchronize();
            return;
        }

        const scopeIdx = this.enterScope(this.prev());
        this.skipNewlines();

        const fields: StructField[] = [];
        while (!this.isAtEnd() && !this.check(TokenType.RBrace)) {
            const type = this.tryParseType();
            if (!type) {
                this.synchronize();
                this.skipNewlines();
                continue;
            }

            if (this.check(TokenType.Identifier)) {
                const fieldName = this.current()!;
                fields.push({
                    type, name: fieldName.lexeme,
                    line: fieldName.line, col: fieldName.col,
                });
                this.references.push({
                    name: fieldName.lexeme, line: fieldName.line, col: fieldName.col,
                    length: fieldName.length, isDefinition: true, isMemberAccess: false,
                });
                this.advance();
            }
            this.skipNewlines();
        }

        if (this.match(TokenType.RBrace)) {
            this.closeScope(scopeIdx, this.prev());
        } else {
            this.closeScope(scopeIdx, this.tokens[this.pos - 1] || nameToken);
        }

        // Update the pre-registered symbol with fields
        if (existingSym && existingSym.isStruct) {
            existingSym.fields = fields;
        }
    }

    // ─── Function Definition ────────────────────────────────────────

    private parseFunctionDef(): void {
        this.advance(); // skip 'command'
        this.skipNewlines();

        if (!this.check(TokenType.Identifier)) {
            this.synchronize();
            return;
        }

        const nameToken = this.current()!;
        const existingSym = this.symTable.lookup(nameToken.lexeme);
        this.advance();

        if (!this.match(TokenType.LParen)) {
            this.synchronize();
            return;
        }

        const scopeIdx = this.enterScope(this.prev());
        this.symTable.enterScope();

        // Parse parameters
        const params: Parameter[] = [];
        while (!this.isAtEnd() && !this.check(TokenType.RParen)) {
            const type = this.tryParseType();
            if (!type) break;

            if (this.check(TokenType.Identifier)) {
                const paramName = this.current()!;
                params.push({
                    type, name: paramName.lexeme,
                    line: paramName.line, col: paramName.col,
                });

                const paramSym: SymbolInfo = {
                    name: paramName.lexeme, type,
                    isConst: false, isFunction: false, isStruct: false, isParameter: true,
                    line: paramName.line, col: paramName.col,
                    endLine: paramName.line, endCol: paramName.col + paramName.length,
                    params: [], fields: [], scopeDepth: this.symTable.depth(),
                };
                this.symTable.declare(paramSym);
                this.symbols.push(paramSym);
                if (scopeIdx >= 0 && scopeIdx < this.scopes.length) {
                    this.scopes[scopeIdx].symbols.push(paramSym);
                }
                this.references.push({
                    name: paramName.lexeme, line: paramName.line, col: paramName.col,
                    length: paramName.length, isDefinition: true, isMemberAccess: false,
                });
                this.advance();
            }

            if (!this.match(TokenType.Comma)) break;
        }

        this.match(TokenType.RParen);

        // Parse return type
        let returnType: TypeSpec = makePrimitive('void');
        if (this.match(TokenType.Arrow)) {
            const rt = this.tryParseType();
            if (rt) returnType = rt;
        }

        // Update pre-registered symbol
        if (existingSym && existingSym.isFunction) {
            existingSym.params = params;
            existingSym.returnType = returnType;
        }

        this.skipNewlines();

        // Parse body
        if (this.check(TokenType.LBrace)) {
            this.parseBlock();
        }

        this.symTable.exitScope();
        if (scopeIdx >= 0) {
            this.closeScope(scopeIdx, this.tokens[this.pos - 1] || nameToken);
        }
    }

    // ─── Regime ─────────────────────────────────────────────────────

    private parseRegime(): void {
        const regimeToken = this.current()!;
        this.advance(); // skip 'regime'
        this.skipNewlines();

        if (this.check(TokenType.Identifier) && this.current()!.lexeme === 'start') {
            this.advance();
            this.hasRegime = true;
        } else {
            this.diagnostics.push({
                line: regimeToken.line, col: regimeToken.col,
                endLine: regimeToken.line, endCol: regimeToken.col + regimeToken.length,
                message: "'regime' must be followed by 'start'.",
                severity: 'error',
                code: 'regime-missing-start',
            });
        }

        this.skipNewlines();
        if (this.check(TokenType.LBrace)) {
            this.symTable.enterScope();
            this.parseBlock();
            this.symTable.exitScope();
        }
    }

    // ─── Import ─────────────────────────────────────────────────────

    private parseImport(): void {
        this.advance(); // skip 'import'
        this.skipNewlines();

        if (this.check(TokenType.StringLiteral)) {
            const pathToken = this.current()!;
            // Strip quotes
            const raw = pathToken.lexeme;
            const path = raw.startsWith('"') && raw.endsWith('"')
                ? raw.slice(1, -1) : raw;
            this.importPaths.push(path);
            this.advance();
        }
        this.expectStatementEnd();
    }

    // ─── Block ──────────────────────────────────────────────────────

    private parseBlock(): void {
        if (!this.match(TokenType.LBrace)) return;

        const scopeIdx = this.enterScope(this.prev());
        this.skipNewlines();

        while (!this.isAtEnd() && !this.check(TokenType.RBrace)) {
            this.parseStatement();
            this.skipNewlines();
        }

        if (this.match(TokenType.RBrace)) {
            this.closeScope(scopeIdx, this.prev());
        } else {
            this.closeScope(scopeIdx, this.tokens[this.pos - 1] || this.tokens[0]);
        }
    }

    // ─── Statement ──────────────────────────────────────────────────

    private parseStatement(): void {
        const t = this.current();
        if (!t) return;

        // Check for C++ keywords
        if (t.type === TokenType.Identifier && CPP_ONLY_KEYWORDS.has(t.lexeme)) {
            const suggestion = CPP_KEYWORD_SUGGESTIONS[t.lexeme];
            const msg = suggestion
                ? `'${t.lexeme}' is not a DictatorScript keyword. Did you mean '${suggestion}'?`
                : `'${t.lexeme}' is not a DictatorScript keyword.`;
            this.diagnostics.push({
                line: t.line, col: t.col,
                endLine: t.line, endCol: t.col + t.length,
                message: msg,
                severity: 'warning',
                code: 'cpp-keyword',
                data: suggestion ? { replacement: suggestion } : undefined,
            });
        }

        switch (t.type) {
            case TokenType.KW_Declare:
                this.parseVarDecl(true);
                break;
            case TokenType.KW_Elite:
                this.parseVarDecl(false);
                break;
            case TokenType.KW_Interrogate:
                this.parseIf();
                break;
            case TokenType.KW_Impose:
                this.parseImpose();
                break;
            case TokenType.KW_Silence:
                if (this.loopDepth === 0) {
                    this.diagnostics.push({
                        line: t.line, col: t.col,
                        endLine: t.line, endCol: t.col + t.length,
                        message: "'silence' (break) used outside of a loop.",
                        severity: 'error',
                    });
                }
                this.advance();
                this.expectStatementEnd();
                break;
            case TokenType.KW_Proceed:
                if (this.loopDepth === 0) {
                    this.diagnostics.push({
                        line: t.line, col: t.col,
                        endLine: t.line, endCol: t.col + t.length,
                        message: "'proceed' (continue) used outside of a loop.",
                        severity: 'error',
                    });
                }
                this.advance();
                this.expectStatementEnd();
                break;
            case TokenType.KW_Report:
                this.advance();
                this.skipExpression();
                this.expectStatementEnd();
                break;
            case TokenType.KW_Broadcast:
                this.advance();
                if (this.match(TokenType.LParen)) {
                    this.skipExpression();
                    this.match(TokenType.RParen);
                }
                this.expectStatementEnd();
                break;
            case TokenType.KW_Demand:
                this.advance();
                if (this.match(TokenType.LParen)) {
                    this.skipExpressionRecordingRefs();
                    this.match(TokenType.RParen);
                }
                this.expectStatementEnd();
                break;
            case TokenType.KW_Kill:
                this.advance();
                this.skipExpressionRecordingRefs();
                this.expectStatementEnd();
                break;
            default:
                if (this.looksLikeDeclaration()) {
                    this.parseVarDecl(false);
                } else {
                    this.skipExpressionRecordingRefs();
                    this.expectStatementEnd();
                }
                break;
        }
    }

    // ─── Variable Declaration ───────────────────────────────────────

    private parseVarDecl(hasDeclare: boolean): void {
        if (hasDeclare) this.advance(); // skip 'declare'

        let isConst = false;
        if (this.check(TokenType.KW_Elite)) {
            isConst = true;
            this.advance();
        }

        const type = this.tryParseType();
        if (!type) {
            this.synchronize();
            return;
        }

        if (!this.check(TokenType.Identifier)) {
            this.synchronize();
            return;
        }

        const nameToken = this.current()!;
        this.advance();

        const sym: SymbolInfo = {
            name: nameToken.lexeme, type, isConst,
            isFunction: false, isStruct: false, isParameter: false,
            line: nameToken.line, col: nameToken.col,
            endLine: nameToken.line, endCol: nameToken.col + nameToken.length,
            params: [], fields: [], scopeDepth: this.symTable.depth(),
        };

        this.symTable.declare(sym);
        this.symbols.push(sym);
        const currentScopeIdx = this.scopeStack[this.scopeStack.length - 1];
        if (currentScopeIdx !== undefined && currentScopeIdx < this.scopes.length) {
            this.scopes[currentScopeIdx].symbols.push(sym);
        }
        this.references.push({
            name: nameToken.lexeme, line: nameToken.line, col: nameToken.col,
            length: nameToken.length, isDefinition: true, isMemberAccess: false,
        });

        // Skip initializer if present
        if (this.match(TokenType.Assign)) {
            this.skipExpressionRecordingRefs();
        }

        this.expectStatementEnd();
    }

    // ─── If / Interrogate ───────────────────────────────────────────

    private parseIf(): void {
        this.advance(); // skip 'interrogate'
        if (this.match(TokenType.LParen)) {
            this.skipExpressionRecordingRefs();
            this.match(TokenType.RParen);
        }
        this.skipNewlines();

        if (this.check(TokenType.LBrace)) {
            this.symTable.enterScope();
            this.parseBlock();
            this.symTable.exitScope();
        }

        this.skipNewlines();

        if (this.check(TokenType.KW_Otherwise)) {
            this.advance();
            this.skipNewlines();

            if (this.check(TokenType.KW_Interrogate)) {
                this.parseIf(); // else-if chain
            } else if (this.check(TokenType.LBrace)) {
                this.symTable.enterScope();
                this.parseBlock();
                this.symTable.exitScope();
            }
        }
    }

    // ─── Impose (loops) ─────────────────────────────────────────────

    private parseImpose(): void {
        this.advance(); // skip 'impose'
        this.skipNewlines();

        if (this.check(TokenType.KW_Forever)) {
            // impose forever { ... }
            this.advance();
            this.skipNewlines();
            this.loopDepth++;
            if (this.check(TokenType.LBrace)) {
                this.symTable.enterScope();
                this.parseBlock();
                this.symTable.exitScope();
            }
            this.loopDepth--;
            return;
        }

        if (!this.match(TokenType.LParen)) return;

        // Check if this is a typed loop (for-range or for-each)
        if (this.looksLikeDeclaration()) {
            this.symTable.enterScope();

            const type = this.tryParseType();
            if (type && this.check(TokenType.Identifier)) {
                const varToken = this.current()!;
                const loopVar: SymbolInfo = {
                    name: varToken.lexeme, type,
                    isConst: false, isFunction: false, isStruct: false, isParameter: false,
                    line: varToken.line, col: varToken.col,
                    endLine: varToken.line, endCol: varToken.col + varToken.length,
                    params: [], fields: [], scopeDepth: this.symTable.depth(),
                };
                this.symTable.declare(loopVar);
                this.symbols.push(loopVar);
                this.references.push({
                    name: varToken.lexeme, line: varToken.line, col: varToken.col,
                    length: varToken.length, isDefinition: true, isMemberAccess: false,
                });
                this.advance();
            }

            this.match(TokenType.KW_From);
            this.skipExpressionRecordingRefs();
            this.match(TokenType.RParen);
            this.skipNewlines();

            this.loopDepth++;
            if (this.check(TokenType.LBrace)) {
                this.parseBlock();
            }
            this.loopDepth--;

            this.symTable.exitScope();
        } else {
            // While loop
            this.skipExpressionRecordingRefs();
            this.match(TokenType.RParen);
            this.skipNewlines();

            this.loopDepth++;
            if (this.check(TokenType.LBrace)) {
                this.symTable.enterScope();
                this.parseBlock();
                this.symTable.exitScope();
            }
            this.loopDepth--;
        }
    }

    // ─── Type Parsing ───────────────────────────────────────────────

    private tryParseType(): TypeSpec | null {
        const t = this.current();
        if (!t) return null;

        let baseType: TypeSpec | null = null;

        if (isTypeKeyword(t.type)) {
            if (t.type === TokenType.KW_Map) {
                // map<K, V>
                this.advance();
                if (!this.match(TokenType.Lt)) return makePrimitive('map');
                const keyType = this.tryParseType();
                this.match(TokenType.Comma);
                const valType = this.tryParseType();
                this.match(TokenType.Gt);
                return makeMap(keyType || makePrimitive('?'), valType || makePrimitive('?'));
            }
            baseType = makePrimitive(t.lexeme);
            this.advance();
        } else if (t.type === TokenType.Identifier) {
            // Could be a struct type
            const sym = this.symTable.lookup(t.lexeme);
            if (sym && sym.isStruct) {
                baseType = makeStruct(t.lexeme);
                this.references.push({
                    name: t.lexeme, line: t.line, col: t.col,
                    length: t.length, isDefinition: false, isMemberAccess: false,
                });
                this.advance();
            } else {
                return null; // not a type
            }
        } else {
            return null;
        }

        // Check for array suffix []
        if (this.check(TokenType.LBracket) && this.peekType(1) === TokenType.RBracket) {
            this.advance(); // [
            this.advance(); // ]
            return makeArray(baseType);
        }

        // Check for pointer suffix ->
        if (this.check(TokenType.Arrow)) {
            this.advance();
            return makePointer(baseType);
        }

        return baseType;
    }

    // ─── Expression Skipping (with reference recording) ─────────────

    /** Skip past an expression, recording all identifier references. */
    private skipExpressionRecordingRefs(): void {
        let depth = 0;
        while (!this.isAtEnd()) {
            const t = this.current()!;

            // Stop at statement boundaries (unless nested in parens/brackets)
            if (depth === 0 && (
                t.type === TokenType.Newline ||
                t.type === TokenType.Semicolon ||
                t.type === TokenType.RBrace ||
                t.type === TokenType.RParen ||
                t.type === TokenType.RBracket
            )) {
                break;
            }

            if (t.type === TokenType.LParen || t.type === TokenType.LBracket) depth++;
            if (t.type === TokenType.RParen || t.type === TokenType.RBracket) {
                if (depth > 0) depth--;
                else break;
            }

            if (t.type === TokenType.Identifier) {
                // Check if preceded by a dot (member access)
                const prevToken = this.pos > 0 ? this.tokens[this.pos - 1] : null;
                const isMember = prevToken?.type === TokenType.Dot;
                this.references.push({
                    name: t.lexeme, line: t.line, col: t.col,
                    length: t.length, isDefinition: false, isMemberAccess: isMember,
                });
            }

            this.advance();
        }
    }

    /** Skip past an expression without recording references. */
    private skipExpression(): void {
        this.skipExpressionRecordingRefs();
    }

    // ─── Helpers ────────────────────────────────────────────────────

    /** Check if current position looks like a variable declaration (Type Identifier pattern). */
    private looksLikeDeclaration(): boolean {
        const t = this.current();
        if (!t) return false;

        if (t.type === TokenType.KW_Elite) return true;

        if (isTypeKeyword(t.type)) {
            // Type keyword — check next meaningful token
            if (t.type === TokenType.KW_Map) return true; // map<...> must be a type
            const next = this.peekToken(1);
            if (!next) return false;
            // Type[] or Type-> are types; Type Identifier is a declaration
            if (next.type === TokenType.Identifier) return true;
            if (next.type === TokenType.LBracket) return true;
            if (next.type === TokenType.Arrow) return true;
        }

        if (t.type === TokenType.Identifier) {
            // Check if this identifier is a known struct type
            const sym = this.symTable.lookup(t.lexeme);
            if (sym && sym.isStruct) {
                const next = this.peekToken(1);
                if (next && (next.type === TokenType.Identifier ||
                    next.type === TokenType.LBracket ||
                    next.type === TokenType.Arrow)) {
                    return true;
                }
            }
        }

        return false;
    }

    private current(): Token | undefined {
        return this.tokens[this.pos];
    }

    private prev(): Token {
        return this.tokens[this.pos - 1];
    }

    private check(type: TokenType): boolean {
        return !this.isAtEnd() && this.tokens[this.pos].type === type;
    }

    private match(type: TokenType): boolean {
        if (this.check(type)) {
            this.advance();
            return true;
        }
        return false;
    }

    private advance(): Token | undefined {
        if (!this.isAtEnd()) {
            return this.tokens[this.pos++];
        }
        return undefined;
    }

    private isAtEnd(): boolean {
        return this.pos >= this.tokens.length || this.tokens[this.pos].type === TokenType.EOF;
    }

    private peekType(offset: number): TokenType | undefined {
        const idx = this.pos + offset;
        return idx < this.tokens.length ? this.tokens[idx].type : undefined;
    }

    private peekToken(offset: number): Token | undefined {
        const idx = this.pos + offset;
        return idx < this.tokens.length ? this.tokens[idx] : undefined;
    }

    private skipNewlines(): void {
        while (!this.isAtEnd()) {
            const t = this.current()!;
            if (t.type === TokenType.Newline || t.type === TokenType.Semicolon) {
                this.advance();
                if (t.type === TokenType.Newline) {
                    // col is already tracked by the lexer
                }
            } else {
                break;
            }
        }
    }

    private expectStatementEnd(): void {
        if (this.isAtEnd()) return;
        const t = this.current()!;
        if (t.type === TokenType.Newline || t.type === TokenType.Semicolon ||
            t.type === TokenType.RBrace || t.type === TokenType.EOF) {
            return; // Don't consume — let the caller handle it
        }
        // Not a fatal error in LSP; just skip to next line
    }

    private synchronize(): void {
        while (!this.isAtEnd()) {
            const t = this.current()!;
            if (t.type === TokenType.Newline || t.type === TokenType.Semicolon) {
                this.advance();
                return;
            }
            if (t.type === TokenType.RBrace) return; // Don't consume
            // Sync keywords
            if (t.type === TokenType.KW_Law || t.type === TokenType.KW_Command ||
                t.type === TokenType.KW_Regime || t.type === TokenType.KW_Declare ||
                t.type === TokenType.KW_Interrogate || t.type === TokenType.KW_Impose ||
                t.type === TokenType.KW_Report || t.type === TokenType.KW_Broadcast ||
                t.type === TokenType.KW_Demand) {
                return; // Don't consume
            }
            this.advance();
        }
    }

    // ─── Scope Tracking ─────────────────────────────────────────────

    private enterScope(openBrace: Token): number {
        const parentIdx = this.scopeStack[this.scopeStack.length - 1] ?? -1;
        const idx = this.scopes.length;
        this.scopes.push({
            startLine: openBrace.line, startCol: openBrace.col,
            endLine: openBrace.line, endCol: openBrace.col + 1,
            parentIndex: parentIdx, symbols: [],
        });
        this.scopeStack.push(idx);
        return idx;
    }

    private closeScope(idx: number, closeBrace: Token): void {
        if (idx < this.scopes.length) {
            this.scopes[idx].endLine = closeBrace.line;
            this.scopes[idx].endCol = closeBrace.col + closeBrace.length;
        }
        if (this.scopeStack.length > 1) {
            this.scopeStack.pop();
        }
    }
}
