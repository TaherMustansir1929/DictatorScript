/**
 * DictatorScript Token Types
 *
 * Mirrors the C++ TokenType enum from src/compiler/TokenType.h.
 * This is the single source of truth for all token classifications.
 */

export enum TokenType {
    // === Literals ===
    IntLiteral,
    FloatLiteral,
    CharLiteral,
    StringLiteral,

    // === Identifiers ===
    Identifier,

    // === Keywords — Types ===
    KW_Int,
    KW_Float,
    KW_Double,
    KW_Char,
    KW_String,
    KW_Bool,
    KW_Void,

    // === Keywords — Declarations ===
    KW_Declare,
    KW_Elite,
    KW_Law,
    KW_Command,

    // === Keywords — Entry Point ===
    KW_Regime,

    // === Keywords — Boolean Literals ===
    KW_Loyal,
    KW_Traitor,

    // === Keywords — Control Flow ===
    KW_Interrogate,
    KW_Otherwise,
    KW_Impose,
    KW_From,
    KW_Forever,
    KW_Silence,
    KW_Proceed,
    KW_Report,

    // === Keywords — I/O ===
    KW_Broadcast,
    KW_Demand,

    // === Keywords — Memory ===
    KW_Summon,
    KW_Kill,
    KW_Null,

    // === Keywords — Imports ===
    KW_Import,

    // === Keywords — Hashmaps ===
    KW_Map,

    // === Operators — Arithmetic ===
    Plus,
    Minus,
    Star,
    Slash,
    Percent,

    // === Operators — Comparison ===
    Eq,
    Neq,
    Lt,
    Gt,
    Lte,
    Gte,

    // === Operators — Logical ===
    And,
    Or,
    Not,

    // === Operators — Assignment ===
    Assign,
    PlusAssign,
    MinusAssign,
    StarAssign,
    SlashAssign,

    // === Operators — Pointer / Address ===
    At,
    Arrow,
    Deref,

    // === Punctuation ===
    LBrace,
    RBrace,
    LParen,
    RParen,
    LBracket,
    RBracket,
    Semicolon,
    Colon,
    Comma,
    Dot,
    DotDot,

    // === Special ===
    Newline,
    EOF,
    Unknown,
}

/** Map from keyword string to TokenType. */
export const KEYWORD_MAP: ReadonlyMap<string, TokenType> = new Map([
    ['int', TokenType.KW_Int],
    ['float', TokenType.KW_Float],
    ['double', TokenType.KW_Double],
    ['char', TokenType.KW_Char],
    ['string', TokenType.KW_String],
    ['bool', TokenType.KW_Bool],
    ['void', TokenType.KW_Void],
    ['declare', TokenType.KW_Declare],
    ['elite', TokenType.KW_Elite],
    ['law', TokenType.KW_Law],
    ['command', TokenType.KW_Command],
    ['regime', TokenType.KW_Regime],
    ['loyal', TokenType.KW_Loyal],
    ['traitor', TokenType.KW_Traitor],
    ['interrogate', TokenType.KW_Interrogate],
    ['otherwise', TokenType.KW_Otherwise],
    ['impose', TokenType.KW_Impose],
    ['from', TokenType.KW_From],
    ['forever', TokenType.KW_Forever],
    ['silence', TokenType.KW_Silence],
    ['proceed', TokenType.KW_Proceed],
    ['report', TokenType.KW_Report],
    ['broadcast', TokenType.KW_Broadcast],
    ['demand', TokenType.KW_Demand],
    ['summon', TokenType.KW_Summon],
    ['kill', TokenType.KW_Kill],
    ['null', TokenType.KW_Null],
    ['import', TokenType.KW_Import],
    ['map', TokenType.KW_Map],
]);

/** Set of token types that represent type keywords (used to detect type positions). */
export const TYPE_KEYWORD_TYPES: ReadonlySet<TokenType> = new Set([
    TokenType.KW_Int,
    TokenType.KW_Float,
    TokenType.KW_Double,
    TokenType.KW_Char,
    TokenType.KW_String,
    TokenType.KW_Bool,
    TokenType.KW_Void,
    TokenType.KW_Map,
]);

/** Check if a token type is any keyword. */
export function isKeyword(type: TokenType): boolean {
    return type >= TokenType.KW_Int && type <= TokenType.KW_Map;
}

/** Check if a token type is a type keyword. */
export function isTypeKeyword(type: TokenType): boolean {
    return TYPE_KEYWORD_TYPES.has(type);
}

/** Check if a token type is an assignment operator. */
export function isAssignmentOp(type: TokenType): boolean {
    return type === TokenType.Assign ||
        type === TokenType.PlusAssign ||
        type === TokenType.MinusAssign ||
        type === TokenType.StarAssign ||
        type === TokenType.SlashAssign;
}
