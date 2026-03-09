#ifndef DICTATORSCRIPT_TOKEN_H
#define DICTATORSCRIPT_TOKEN_H

#include "TokenType.h"
#include <string>

// Token represents a single lexical token produced by the Lexer.
// Each token carries its type, original text (lexeme), and source location.
struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int col;

    Token();
    Token(TokenType type, const std::string& lexeme, int line, int col);

    // Human-readable representation for debug output (--dump-tokens).
    std::string toString() const;
};

#endif // DICTATORSCRIPT_TOKEN_H
