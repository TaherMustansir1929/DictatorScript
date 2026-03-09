/**
 * DictatorScript Lexer
 *
 * TypeScript port of src/compiler/Lexer.cpp.
 * Tokenizes DictatorScript source into a stream of tokens.
 * Positions are 0-based (LSP convention).
 */

import { TokenType, KEYWORD_MAP } from '../../shared/src/tokenType';
import { Token } from '../../shared/src/token';
import { LexerDiagnostic } from '../../shared/src/types';
import { VALID_ESCAPE_CHARS } from '../../shared/src/languageData';

export interface LexResult {
    tokens: Token[];
    diagnostics: LexerDiagnostic[];
}

export function tokenize(source: string): LexResult {
    const lexer = new Lexer(source);
    return lexer.tokenize();
}

class Lexer {
    private source: string;
    private pos = 0;
    private line = 0;
    private col = 0;
    private tokens: Token[] = [];
    private diagnostics: LexerDiagnostic[] = [];

    constructor(source: string) {
        this.source = source;
    }

    tokenize(): LexResult {
        while (!this.isAtEnd()) {
            this.scanToken();
        }
        this.addToken(TokenType.EOF, '', this.pos, 0);
        return { tokens: this.tokens, diagnostics: this.diagnostics };
    }

    // ─── Core scanning ─────────────────────────────────────────────

    private scanToken(): void {
        this.skipWhitespace();
        if (this.isAtEnd()) return;

        const c = this.current();

        // Newline
        if (c === '\n') {
            const start = this.pos;
            this.addToken(TokenType.Newline, '\n', start, 1);
            this.advance();
            this.line++;
            this.col = 0;
            return;
        }

        // Comments
        if (c === '/' && this.pos + 1 < this.source.length) {
            if (this.peek() === '/') {
                this.skipLineComment();
                return;
            }
            if (this.peek() === '*') {
                this.skipBlockComment();
                return;
            }
        }

        // String literal
        if (c === '"') {
            this.scanString();
            return;
        }

        // Char literal
        if (c === "'") {
            this.scanChar();
            return;
        }

        // Number literal
        if (this.isDigit(c)) {
            this.scanNumber();
            return;
        }

        // Identifier or keyword
        if (this.isAlpha(c)) {
            this.scanIdentifierOrKeyword();
            return;
        }

        // Operators and punctuation
        this.scanOperatorOrPunct();
    }

    // ─── Whitespace & Comments ──────────────────────────────────────

    private skipWhitespace(): void {
        while (!this.isAtEnd()) {
            const c = this.current();
            if (c === ' ' || c === '\t' || c === '\r') {
                this.advance();
            } else {
                break;
            }
        }
    }

    private skipLineComment(): void {
        // Skip //
        this.advance();
        this.advance();
        while (!this.isAtEnd() && this.current() !== '\n') {
            this.advance();
        }
        // Don't consume the newline — it will be emitted as TOKEN_NEWLINE
    }

    private skipBlockComment(): void {
        const startLine = this.line;
        const startCol = this.col;
        // Skip /*
        this.advance();
        this.advance();

        while (!this.isAtEnd()) {
            if (this.current() === '*' && this.pos + 1 < this.source.length && this.peek() === '/') {
                this.advance(); // *
                this.advance(); // /
                return;
            }
            if (this.current() === '\n') {
                this.line++;
                this.col = 0;
                this.pos++;
                continue;
            }
            this.advance();
        }

        // Unterminated
        this.diagnostics.push({
            line: startLine,
            col: startCol,
            endCol: startCol + 2,
            message: 'Unterminated block comment.',
            severity: 'error',
        });
    }

    // ─── String Literal ─────────────────────────────────────────────

    private scanString(): void {
        const startPos = this.pos;
        const startCol = this.col;
        const startLine = this.line;
        this.advance(); // skip opening "

        let value = '';

        while (!this.isAtEnd() && this.current() !== '"') {
            if (this.current() === '\n') {
                this.diagnostics.push({
                    line: startLine,
                    col: startCol,
                    endCol: this.col,
                    message: 'Unterminated string literal.',
                    severity: 'error',
                });
                // Don't consume the newline
                const lexeme = this.source.substring(startPos, this.pos);
                this.addToken(TokenType.StringLiteral, lexeme, startPos, this.pos - startPos);
                return;
            }
            if (this.current() === '\\') {
                this.advance(); // skip backslash
                if (this.isAtEnd()) break;
                const esc = this.current();
                if (VALID_ESCAPE_CHARS.has(esc)) {
                    switch (esc) {
                        case 'n': value += '\n'; break;
                        case 't': value += '\t'; break;
                        case '\\': value += '\\'; break;
                        case '"': value += '"'; break;
                        case '0': value += '\0'; break;
                        default: value += esc;
                    }
                } else {
                    this.diagnostics.push({
                        line: this.line,
                        col: this.col - 1,
                        endCol: this.col + 1,
                        message: `Unknown escape sequence: '\\${esc}'.`,
                        severity: 'warning',
                    });
                    value += esc;
                }
                this.advance();
            } else {
                value += this.current();
                this.advance();
            }
        }

        if (this.isAtEnd()) {
            this.diagnostics.push({
                line: startLine,
                col: startCol,
                endCol: this.col,
                message: 'Unterminated string literal.',
                severity: 'error',
            });
        } else {
            this.advance(); // skip closing "
        }

        const lexeme = this.source.substring(startPos, this.pos);
        this.addToken(TokenType.StringLiteral, lexeme, startPos, this.pos - startPos);
    }

    // ─── Char Literal ───────────────────────────────────────────────

    private scanChar(): void {
        const startPos = this.pos;
        const startCol = this.col;
        const startLine = this.line;
        this.advance(); // skip opening '

        if (this.isAtEnd() || this.current() === '\n') {
            this.diagnostics.push({
                line: startLine,
                col: startCol,
                endCol: this.col,
                message: 'Unterminated character literal.',
                severity: 'error',
            });
            const lexeme = this.source.substring(startPos, this.pos);
            this.addToken(TokenType.CharLiteral, lexeme, startPos, this.pos - startPos);
            return;
        }

        if (this.current() === '\\') {
            this.advance(); // skip backslash
            if (!this.isAtEnd()) {
                const esc = this.current();
                if (!VALID_ESCAPE_CHARS.has(esc)) {
                    this.diagnostics.push({
                        line: this.line,
                        col: this.col - 1,
                        endCol: this.col + 1,
                        message: `Unknown escape sequence: '\\${esc}'.`,
                        severity: 'warning',
                    });
                }
                this.advance();
            }
        } else {
            this.advance(); // skip the character
        }

        if (this.isAtEnd() || this.current() !== "'") {
            this.diagnostics.push({
                line: startLine,
                col: startCol,
                endCol: this.col,
                message: 'Unterminated character literal.',
                severity: 'error',
            });
        } else {
            this.advance(); // skip closing '
        }

        const lexeme = this.source.substring(startPos, this.pos);
        this.addToken(TokenType.CharLiteral, lexeme, startPos, this.pos - startPos);
    }

    // ─── Number Literal ─────────────────────────────────────────────

    private scanNumber(): void {
        const startPos = this.pos;
        let isFloat = false;

        while (!this.isAtEnd() && this.isDigit(this.current())) {
            this.advance();
        }

        // Check for decimal point (but NOT range operator ..)
        if (!this.isAtEnd() && this.current() === '.' &&
            this.pos + 1 < this.source.length && this.peek() !== '.') {
            if (this.isDigit(this.peek())) {
                isFloat = true;
                this.advance(); // skip .
                while (!this.isAtEnd() && this.isDigit(this.current())) {
                    this.advance();
                }
            }
        }

        const lexeme = this.source.substring(startPos, this.pos);
        this.addToken(
            isFloat ? TokenType.FloatLiteral : TokenType.IntLiteral,
            lexeme,
            startPos,
            this.pos - startPos
        );
    }

    // ─── Identifier / Keyword ───────────────────────────────────────

    private scanIdentifierOrKeyword(): void {
        const startPos = this.pos;

        while (!this.isAtEnd() && this.isAlphaNumeric(this.current())) {
            this.advance();
        }

        const lexeme = this.source.substring(startPos, this.pos);
        const keywordType = KEYWORD_MAP.get(lexeme);
        this.addToken(
            keywordType !== undefined ? keywordType : TokenType.Identifier,
            lexeme,
            startPos,
            this.pos - startPos
        );
    }

    // ─── Operators & Punctuation ────────────────────────────────────

    private scanOperatorOrPunct(): void {
        const startPos = this.pos;
        const c = this.current();
        this.advance();

        switch (c) {
            case '+':
                if (!this.isAtEnd() && this.current() === '=') {
                    this.advance();
                    this.addToken(TokenType.PlusAssign, '+=', startPos, 2);
                } else {
                    this.addToken(TokenType.Plus, '+', startPos, 1);
                }
                break;

            case '-':
                if (!this.isAtEnd() && this.current() === '>') {
                    this.advance();
                    this.addToken(TokenType.Arrow, '->', startPos, 2);
                } else if (!this.isAtEnd() && this.current() === '=') {
                    this.advance();
                    this.addToken(TokenType.MinusAssign, '-=', startPos, 2);
                } else {
                    this.addToken(TokenType.Minus, '-', startPos, 1);
                }
                break;

            case '*':
                if (!this.isAtEnd() && this.current() === '=') {
                    this.advance();
                    this.addToken(TokenType.StarAssign, '*=', startPos, 2);
                } else {
                    this.addToken(TokenType.Star, '*', startPos, 1);
                }
                break;

            case '/':
                if (!this.isAtEnd() && this.current() === '=') {
                    this.advance();
                    this.addToken(TokenType.SlashAssign, '/=', startPos, 2);
                } else {
                    this.addToken(TokenType.Slash, '/', startPos, 1);
                }
                break;

            case '%':
                this.addToken(TokenType.Percent, '%', startPos, 1);
                break;

            case '=':
                if (!this.isAtEnd() && this.current() === '=') {
                    this.advance();
                    this.addToken(TokenType.Eq, '==', startPos, 2);
                } else {
                    this.addToken(TokenType.Assign, '=', startPos, 1);
                }
                break;

            case '!':
                if (!this.isAtEnd() && this.current() === '=') {
                    this.advance();
                    this.addToken(TokenType.Neq, '!=', startPos, 2);
                } else {
                    this.addToken(TokenType.Not, '!', startPos, 1);
                }
                break;

            case '<':
                if (!this.isAtEnd() && this.current() === '=') {
                    this.advance();
                    this.addToken(TokenType.Lte, '<=', startPos, 2);
                } else {
                    this.addToken(TokenType.Lt, '<', startPos, 1);
                }
                break;

            case '>':
                if (!this.isAtEnd() && this.current() === '=') {
                    this.advance();
                    this.addToken(TokenType.Gte, '>=', startPos, 2);
                } else {
                    this.addToken(TokenType.Gt, '>', startPos, 1);
                }
                break;

            case '&':
                if (!this.isAtEnd() && this.current() === '&') {
                    this.advance();
                    this.addToken(TokenType.And, '&&', startPos, 2);
                } else {
                    this.diagnostics.push({
                        line: this.line,
                        col: this.col - 1,
                        endCol: this.col,
                        message: "Unexpected '&'. Use '@' for address-of, '&&' for logical AND.",
                        severity: 'error',
                    });
                    this.addToken(TokenType.Unknown, '&', startPos, 1);
                }
                break;

            case '|':
                if (!this.isAtEnd() && this.current() === '|') {
                    this.advance();
                    this.addToken(TokenType.Or, '||', startPos, 2);
                } else {
                    this.diagnostics.push({
                        line: this.line,
                        col: this.col - 1,
                        endCol: this.col,
                        message: "Unexpected '|'. Use '||' for logical OR.",
                        severity: 'error',
                    });
                    this.addToken(TokenType.Unknown, '|', startPos, 1);
                }
                break;

            case '@':
                this.addToken(TokenType.At, '@', startPos, 1);
                break;

            case '^':
                this.addToken(TokenType.Deref, '^', startPos, 1);
                break;

            case '{':
                this.addToken(TokenType.LBrace, '{', startPos, 1);
                break;

            case '}':
                this.addToken(TokenType.RBrace, '}', startPos, 1);
                break;

            case '(':
                this.addToken(TokenType.LParen, '(', startPos, 1);
                break;

            case ')':
                this.addToken(TokenType.RParen, ')', startPos, 1);
                break;

            case '[':
                this.addToken(TokenType.LBracket, '[', startPos, 1);
                break;

            case ']':
                this.addToken(TokenType.RBracket, ']', startPos, 1);
                break;

            case ';':
                this.addToken(TokenType.Semicolon, ';', startPos, 1);
                break;

            case ':':
                this.addToken(TokenType.Colon, ':', startPos, 1);
                break;

            case ',':
                this.addToken(TokenType.Comma, ',', startPos, 1);
                break;

            case '.':
                if (!this.isAtEnd() && this.current() === '.') {
                    this.advance();
                    this.addToken(TokenType.DotDot, '..', startPos, 2);
                } else {
                    this.addToken(TokenType.Dot, '.', startPos, 1);
                }
                break;

            default:
                this.diagnostics.push({
                    line: this.line,
                    col: this.col - 1,
                    endCol: this.col,
                    message: `Unexpected character: '${c}'.`,
                    severity: 'error',
                });
                this.addToken(TokenType.Unknown, c, startPos, 1);
                break;
        }
    }

    // ─── Helpers ────────────────────────────────────────────────────

    private current(): string {
        return this.source[this.pos];
    }

    private peek(): string {
        return this.source[this.pos + 1];
    }

    private advance(): void {
        this.pos++;
        this.col++;
    }

    private isAtEnd(): boolean {
        return this.pos >= this.source.length;
    }

    private isDigit(c: string): boolean {
        return c >= '0' && c <= '9';
    }

    private isAlpha(c: string): boolean {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c === '_';
    }

    private isAlphaNumeric(c: string): boolean {
        return this.isAlpha(c) || this.isDigit(c);
    }

    private addToken(type: TokenType, lexeme: string, offset: number, length: number): void {
        this.tokens.push({
            type,
            lexeme,
            line: type === TokenType.Newline ? this.line : this.line,
            col: this.col - length,
            offset,
            length,
        });
    }
}
