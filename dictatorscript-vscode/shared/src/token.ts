/**
 * DictatorScript Token
 *
 * Represents a single token produced by the lexer.
 * Positions are 0-based (LSP convention).
 */

import { TokenType } from './tokenType';

export interface Token {
    /** The token classification. */
    type: TokenType;
    /** The raw source text of the token. */
    lexeme: string;
    /** 0-based line number. */
    line: number;
    /** 0-based column number. */
    col: number;
    /** Absolute character offset from the start of the source. */
    offset: number;
    /** Length of the token in characters. */
    length: number;
}
