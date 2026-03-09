#include "Token.h"

Token::Token() : type(TokenType::TOKEN_UNKNOWN), lexeme(""), line(0), col(0) {}

Token::Token(TokenType type, const std::string& lexeme, int line, int col)
    : type(type), lexeme(lexeme), line(line), col(col) {}

std::string Token::toString() const {
    return "[" + tokenTypeToString(type) + " \"" + lexeme + "\" L:" +
           std::to_string(line) + " C:" + std::to_string(col) + "]";
}
