#ifndef DICTATORSCRIPT_TOKEN_TYPE_H
#define DICTATORSCRIPT_TOKEN_TYPE_H

#include <string>

// TokenType enumeration covering every keyword, operator, punctuation, and literal type
// in DictatorScript. To add a new token type, add an entry here and update
// tokenTypeToString() below.

enum class TokenType {
    // Literals
    TOKEN_INT_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_CHAR_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_BOOL_LITERAL,

    // Identifiers
    TOKEN_IDENTIFIER,

    // Keywords — Types
    KW_INT,
    KW_FLOAT,
    KW_DOUBLE,
    KW_CHAR,
    KW_STRING,
    KW_BOOL,
    KW_VOID,

    // Keywords — Declarations
    KW_DECLARE,
    KW_ELITE,
    KW_LAW,
    KW_COMMAND,

    // Keywords — Entry Point
    KW_REGIME,

    // Keywords — Boolean Literals
    KW_LOYAL,    // true
    KW_TRAITOR,  // false

    // Keywords — Control Flow
    KW_INTERROGATE,   // if
    KW_OTHERWISE,     // else / else if
    KW_IMPOSE,        // for / while
    KW_FROM,          // range/iteration keyword
    KW_FOREVER,       // infinite loop
    KW_SILENCE,       // break
    KW_PROCEED,       // continue
    KW_REPORT,        // return

    // Keywords — I/O
    KW_BROADCAST,     // cout / print
    KW_DEMAND,        // cin / input

    // Keywords — Memory
    KW_SUMMON,        // new (dynamic allocation)
    KW_KILL,          // delete (free dynamic memory)
    KW_NULL_VAL,      // nullptr

    // Keywords — Multi-file / Imports
    KW_ENLIST,        // #include / import for other .ds files

    // Keywords — Hashmaps
    KW_MAP,           // map/hashmap type keyword

    // Operators — Arithmetic
    OP_PLUS,
    OP_MINUS,
    OP_STAR,
    OP_SLASH,
    OP_PERCENT,

    // Operators — Comparison
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_GT,
    OP_LTE,
    OP_GTE,

    // Operators — Logical
    OP_AND,
    OP_OR,
    OP_NOT,

    // Operators — Assignment
    OP_ASSIGN,
    OP_PLUS_ASSIGN,
    OP_MINUS_ASSIGN,
    OP_STAR_ASSIGN,
    OP_SLASH_ASSIGN,

    // Operators — Pointer / Address
    OP_AT,            // @ — address-of (replaces &)
    OP_ARROW,         // -> — return type annotation & pointer type
    OP_DEREF,         // ^ — dereference operator (replaces *)

    // Punctuation
    PUNCT_LBRACE,       // {
    PUNCT_RBRACE,       // }
    PUNCT_LPAREN,       // (
    PUNCT_RPAREN,       // )
    PUNCT_LBRACKET,     // [
    PUNCT_RBRACKET,     // ]
    PUNCT_SEMICOLON,    // ;
    PUNCT_COLON,        // :
    PUNCT_COMMA,        // ,
    PUNCT_DOT,          // .
    PUNCT_DOTDOT,       // ..

    // Special
    TOKEN_EOF,
    TOKEN_NEWLINE,
    TOKEN_UNKNOWN
};

// Convert a TokenType to its string name for debug output.
// Extension point: add new entries when new token types are added.
inline std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::TOKEN_INT_LITERAL:   return "TOKEN_INT_LITERAL";
        case TokenType::TOKEN_FLOAT_LITERAL: return "TOKEN_FLOAT_LITERAL";
        case TokenType::TOKEN_CHAR_LITERAL:  return "TOKEN_CHAR_LITERAL";
        case TokenType::TOKEN_STRING_LITERAL:return "TOKEN_STRING_LITERAL";
        case TokenType::TOKEN_BOOL_LITERAL:  return "TOKEN_BOOL_LITERAL";
        case TokenType::TOKEN_IDENTIFIER:    return "TOKEN_IDENTIFIER";
        case TokenType::KW_INT:              return "KW_INT";
        case TokenType::KW_FLOAT:            return "KW_FLOAT";
        case TokenType::KW_DOUBLE:           return "KW_DOUBLE";
        case TokenType::KW_CHAR:             return "KW_CHAR";
        case TokenType::KW_STRING:           return "KW_STRING";
        case TokenType::KW_BOOL:             return "KW_BOOL";
        case TokenType::KW_VOID:             return "KW_VOID";
        case TokenType::KW_DECLARE:          return "KW_DECLARE";
        case TokenType::KW_ELITE:            return "KW_ELITE";
        case TokenType::KW_LAW:              return "KW_LAW";
        case TokenType::KW_COMMAND:          return "KW_COMMAND";
        case TokenType::KW_REGIME:           return "KW_REGIME";
        case TokenType::KW_LOYAL:            return "KW_LOYAL";
        case TokenType::KW_TRAITOR:          return "KW_TRAITOR";
        case TokenType::KW_INTERROGATE:      return "KW_INTERROGATE";
        case TokenType::KW_OTHERWISE:        return "KW_OTHERWISE";
        case TokenType::KW_IMPOSE:           return "KW_IMPOSE";
        case TokenType::KW_FROM:             return "KW_FROM";
        case TokenType::KW_FOREVER:          return "KW_FOREVER";
        case TokenType::KW_SILENCE:          return "KW_SILENCE";
        case TokenType::KW_PROCEED:          return "KW_PROCEED";
        case TokenType::KW_REPORT:           return "KW_REPORT";
        case TokenType::KW_BROADCAST:        return "KW_BROADCAST";
        case TokenType::KW_DEMAND:           return "KW_DEMAND";
        case TokenType::KW_SUMMON:           return "KW_SUMMON";
        case TokenType::KW_KILL:             return "KW_KILL";
        case TokenType::KW_NULL_VAL:         return "KW_NULL_VAL";
        case TokenType::KW_ENLIST:           return "KW_ENLIST";
        case TokenType::KW_MAP:              return "KW_MAP";
        case TokenType::OP_PLUS:             return "OP_PLUS";
        case TokenType::OP_MINUS:            return "OP_MINUS";
        case TokenType::OP_STAR:             return "OP_STAR";
        case TokenType::OP_SLASH:            return "OP_SLASH";
        case TokenType::OP_PERCENT:          return "OP_PERCENT";
        case TokenType::OP_EQ:              return "OP_EQ";
        case TokenType::OP_NEQ:             return "OP_NEQ";
        case TokenType::OP_LT:              return "OP_LT";
        case TokenType::OP_GT:              return "OP_GT";
        case TokenType::OP_LTE:             return "OP_LTE";
        case TokenType::OP_GTE:             return "OP_GTE";
        case TokenType::OP_AND:             return "OP_AND";
        case TokenType::OP_OR:              return "OP_OR";
        case TokenType::OP_NOT:             return "OP_NOT";
        case TokenType::OP_ASSIGN:          return "OP_ASSIGN";
        case TokenType::OP_PLUS_ASSIGN:     return "OP_PLUS_ASSIGN";
        case TokenType::OP_MINUS_ASSIGN:    return "OP_MINUS_ASSIGN";
        case TokenType::OP_STAR_ASSIGN:     return "OP_STAR_ASSIGN";
        case TokenType::OP_SLASH_ASSIGN:    return "OP_SLASH_ASSIGN";
        case TokenType::OP_AT:             return "OP_AT";
        case TokenType::OP_ARROW:          return "OP_ARROW";
        case TokenType::OP_DEREF:          return "OP_DEREF";
        case TokenType::PUNCT_LBRACE:      return "PUNCT_LBRACE";
        case TokenType::PUNCT_RBRACE:      return "PUNCT_RBRACE";
        case TokenType::PUNCT_LPAREN:      return "PUNCT_LPAREN";
        case TokenType::PUNCT_RPAREN:      return "PUNCT_RPAREN";
        case TokenType::PUNCT_LBRACKET:    return "PUNCT_LBRACKET";
        case TokenType::PUNCT_RBRACKET:    return "PUNCT_RBRACKET";
        case TokenType::PUNCT_SEMICOLON:   return "PUNCT_SEMICOLON";
        case TokenType::PUNCT_COLON:       return "PUNCT_COLON";
        case TokenType::PUNCT_COMMA:       return "PUNCT_COMMA";
        case TokenType::PUNCT_DOT:         return "PUNCT_DOT";
        case TokenType::PUNCT_DOTDOT:      return "PUNCT_DOTDOT";
        case TokenType::TOKEN_EOF:         return "TOKEN_EOF";
        case TokenType::TOKEN_NEWLINE:     return "TOKEN_NEWLINE";
        case TokenType::TOKEN_UNKNOWN:     return "TOKEN_UNKNOWN";
    }
    return "UNKNOWN";
}

#endif // DICTATORSCRIPT_TOKEN_TYPE_H
