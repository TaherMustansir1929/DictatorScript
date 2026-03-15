#include "Lexer.h"
#include <cctype>

Lexer::Lexer(const std::string& source, const std::string& filename,
             ErrorReporter& errorReporter)
    : source_(source), filename_(filename), errors_(errorReporter),
      pos_(0), line_(1), column_(1) {
    initKeywords();
}

void Lexer::initKeywords() {
    // Type keywords
    keywords_["int"]         = TokenType::KW_INT;
    keywords_["float"]       = TokenType::KW_FLOAT;
    keywords_["double"]      = TokenType::KW_DOUBLE;
    keywords_["char"]        = TokenType::KW_CHAR;
    keywords_["string"]      = TokenType::KW_STRING;
    keywords_["bool"]        = TokenType::KW_BOOL;
    keywords_["void"]        = TokenType::KW_VOID;

    // Declaration keywords
    keywords_["declare"]     = TokenType::KW_DECLARE;
    keywords_["elite"]       = TokenType::KW_ELITE;
    keywords_["law"]         = TokenType::KW_LAW;
    keywords_["command"]     = TokenType::KW_COMMAND;

    // Entry point
    keywords_["regime"]      = TokenType::KW_REGIME;

    // Boolean literals
    keywords_["loyal"]       = TokenType::KW_LOYAL;
    keywords_["traitor"]     = TokenType::KW_TRAITOR;

    // Control flow
    keywords_["interrogate"] = TokenType::KW_INTERROGATE;
    keywords_["otherwise"]   = TokenType::KW_OTHERWISE;
    keywords_["impose"]      = TokenType::KW_IMPOSE;
    keywords_["from"]        = TokenType::KW_FROM;
    keywords_["forever"]     = TokenType::KW_FOREVER;
    keywords_["silence"]     = TokenType::KW_SILENCE;
    keywords_["proceed"]     = TokenType::KW_PROCEED;
    keywords_["report"]      = TokenType::KW_REPORT;

    // I/O
    keywords_["broadcast"]   = TokenType::KW_BROADCAST;
    keywords_["demand"]      = TokenType::KW_DEMAND;

    // Memory
    keywords_["summon"]      = TokenType::KW_SUMMON;
    keywords_["kill"]        = TokenType::KW_KILL;
    keywords_["null"]        = TokenType::KW_NULL_VAL;

    // Imports
    keywords_["import"]      = TokenType::KW_ENLIST;

    // Hashmaps
    keywords_["map"]         = TokenType::KW_MAP;

    // Smart Pointers (C++11) — Feature 1
    keywords_["guard"]        = TokenType::KW_GUARD;
    keywords_["share"]        = TokenType::KW_SHARE;
    keywords_["summon_guard"] = TokenType::KW_SUMMON_GUARD;
    keywords_["summon_share"] = TokenType::KW_SUMMON_SHARE;

    // Lambda Expressions (C++11) — Feature 2
    keywords_["block"]       = TokenType::KW_BLOCK;

    // Auto Type Deduction (C++11) — Feature 3
    keywords_["auto"]        = TokenType::KW_AUTO;

    // Concurrency (C++11) — Feature 4
    keywords_["spawn"]       = TokenType::KW_SPAWN;

    // Structured Bindings (C++17) — Feature 5
    keywords_["unpack"]      = TokenType::KW_UNPACK;
}

char Lexer::current() const {
    if (isAtEnd()) return '\0';
    return source_[pos_];
}

char Lexer::peek() const {
    return peekAt(1);
}

char Lexer::peekAt(int offset) const {
    int idx = pos_ + offset;
    if (idx >= (int)source_.size()) return '\0';
    return source_[idx];
}

bool Lexer::isAtEnd() const {
    return pos_ >= (int)source_.size();
}

char Lexer::advance() {
    char c = source_[pos_];
    pos_++;
    column_++;
    return c;
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = current();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '/' && peek() == '/') {
            skipLineComment();
        } else if (c == '/' && peek() == '*') {
            if (!skipBlockComment()) return;
        } else {
            break;
        }
    }
}

void Lexer::skipLineComment() {
    // Skip until end of line (don't consume the newline itself).
    while (!isAtEnd() && current() != '\n') {
        advance();
    }
}

bool Lexer::skipBlockComment() {
    int startLine = line_;
    int startCol = column_;
    advance(); // skip /
    advance(); // skip *
    while (!isAtEnd()) {
        if (current() == '*' && peek() == '/') {
            advance(); // skip *
            advance(); // skip /
            return true;
        }
        if (current() == '\n') {
            line_++;
            column_ = 0; // advance() will set it to 1
        }
        advance();
    }
    errors_.error(filename_, startLine, startCol,
                  "Unterminated block comment.");
    return false;
}

std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        skipWhitespaceAndComments();
        if (isAtEnd()) break;
        scanToken();
    }
    addToken(TokenType::TOKEN_EOF, "", line_, column_);
    return tokens_;
}

void Lexer::scanToken() {
    char c = current();

    if (c == '\n') {
        addToken(TokenType::TOKEN_NEWLINE, "\\n", line_, column_);
        advance();
        line_++;
        column_ = 1;
        return;
    }

    if (c == '"') {
        scanString();
        return;
    }

    if (c == '\'') {
        scanChar();
        return;
    }

    if (std::isdigit(c)) {
        scanNumber();
        return;
    }

    if (std::isalpha(c) || c == '_') {
        scanIdentifierOrKeyword();
        return;
    }

    scanOperatorOrPunct();
}

void Lexer::scanString() {
    int startLine = line_;
    int startCol = column_;
    advance(); // skip opening "
    std::string value;
    while (!isAtEnd() && current() != '"') {
        if (current() == '\n') {
            errors_.error(filename_, startLine, startCol,
                          "Unterminated string literal.");
            return;
        }
        if (current() == '\\') {
            advance();
            switch (current()) {
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                case '\\': value += '\\'; break;
                case '"':  value += '"';  break;
                case '0':  value += '\0'; break;
                default:
                    errors_.warning(filename_, line_, column_,
                                    std::string("Unknown escape sequence: \\") + current());
                    value += current();
                    break;
            }
            advance();
        } else {
            value += current();
            advance();
        }
    }
    if (isAtEnd()) {
        errors_.error(filename_, startLine, startCol,
                      "Unterminated string literal.");
        return;
    }
    advance(); // skip closing "
    addToken(TokenType::TOKEN_STRING_LITERAL, value, startLine, startCol);
}

void Lexer::scanChar() {
    int startLine = line_;
    int startCol = column_;
    advance(); // skip opening '
    std::string value;
    if (isAtEnd() || current() == '\n') {
        errors_.error(filename_, startLine, startCol,
                      "Unterminated character literal.");
        return;
    }
    if (current() == '\\') {
        advance();
        switch (current()) {
            case 'n':  value += '\n'; break;
            case 't':  value += '\t'; break;
            case '\\': value += '\\'; break;
            case '\'': value += '\''; break;
            case '0':  value += '\0'; break;
            default:
                errors_.warning(filename_, line_, column_,
                                std::string("Unknown escape sequence: \\") + current());
                value += current();
                break;
        }
        advance();
    } else {
        value += current();
        advance();
    }
    if (isAtEnd() || current() != '\'') {
        errors_.error(filename_, startLine, startCol,
                      "Unterminated character literal.");
        return;
    }
    advance(); // skip closing '
    addToken(TokenType::TOKEN_CHAR_LITERAL, value, startLine, startCol);
}

void Lexer::scanNumber() {
    int startLine = line_;
    int startCol = column_;
    std::string num;
    bool isFloat = false;

    while (!isAtEnd() && std::isdigit(current())) {
        num += current();
        advance();
    }

    // Check for decimal point (but not .. range operator).
    if (!isAtEnd() && current() == '.' && peek() != '.') {
        isFloat = true;
        num += current();
        advance();
        while (!isAtEnd() && std::isdigit(current())) {
            num += current();
            advance();
        }
    }

    addToken(isFloat ? TokenType::TOKEN_FLOAT_LITERAL : TokenType::TOKEN_INT_LITERAL,
             num, startLine, startCol);
}

void Lexer::scanIdentifierOrKeyword() {
    int startLine = line_;
    int startCol = column_;
    std::string ident;

    while (!isAtEnd() && (std::isalnum(current()) || current() == '_')) {
        ident += current();
        advance();
    }

    auto it = keywords_.find(ident);
    if (it != keywords_.end()) {
        addToken(it->second, ident, startLine, startCol);
    } else {
        addToken(TokenType::TOKEN_IDENTIFIER, ident, startLine, startCol);
    }
}

void Lexer::scanOperatorOrPunct() {
    int startLine = line_;
    int startCol = column_;
    char c = current();

    switch (c) {
        case '+':
            advance();
            if (!isAtEnd() && current() == '=') {
                advance();
                addToken(TokenType::OP_PLUS_ASSIGN, "+=", startLine, startCol);
            } else {
                addToken(TokenType::OP_PLUS, "+", startLine, startCol);
            }
            break;

        case '-':
            advance();
            if (!isAtEnd() && current() == '>') {
                advance();
                addToken(TokenType::OP_ARROW, "->", startLine, startCol);
            } else if (!isAtEnd() && current() == '=') {
                advance();
                addToken(TokenType::OP_MINUS_ASSIGN, "-=", startLine, startCol);
            } else {
                addToken(TokenType::OP_MINUS, "-", startLine, startCol);
            }
            break;

        case '*':
            advance();
            if (!isAtEnd() && current() == '=') {
                advance();
                addToken(TokenType::OP_STAR_ASSIGN, "*=", startLine, startCol);
            } else {
                addToken(TokenType::OP_STAR, "*", startLine, startCol);
            }
            break;

        case '/':
            advance();
            if (!isAtEnd() && current() == '=') {
                advance();
                addToken(TokenType::OP_SLASH_ASSIGN, "/=", startLine, startCol);
            } else {
                addToken(TokenType::OP_SLASH, "/", startLine, startCol);
            }
            break;

        case '%':
            advance();
            addToken(TokenType::OP_PERCENT, "%", startLine, startCol);
            break;

        case '=':
            advance();
            if (!isAtEnd() && current() == '=') {
                advance();
                addToken(TokenType::OP_EQ, "==", startLine, startCol);
            } else {
                addToken(TokenType::OP_ASSIGN, "=", startLine, startCol);
            }
            break;

        case '!':
            advance();
            if (!isAtEnd() && current() == '=') {
                advance();
                addToken(TokenType::OP_NEQ, "!=", startLine, startCol);
            } else {
                addToken(TokenType::OP_NOT, "!", startLine, startCol);
            }
            break;

        case '<':
            advance();
            if (!isAtEnd() && current() == '=') {
                advance();
                addToken(TokenType::OP_LTE, "<=", startLine, startCol);
            } else {
                addToken(TokenType::OP_LT, "<", startLine, startCol);
            }
            break;

        case '>':
            advance();
            if (!isAtEnd() && current() == '=') {
                advance();
                addToken(TokenType::OP_GTE, ">=", startLine, startCol);
            } else {
                addToken(TokenType::OP_GT, ">", startLine, startCol);
            }
            break;

        case '&':
            advance();
            if (!isAtEnd() && current() == '&') {
                advance();
                addToken(TokenType::OP_AND, "&&", startLine, startCol);
            } else {
                errors_.error(filename_, startLine, startCol,
                              "Unexpected '&'. Use '@' for address-of, '&&' for logical AND.");
            }
            break;

        case '|':
            advance();
            if (!isAtEnd() && current() == '|') {
                advance();
                addToken(TokenType::OP_OR, "||", startLine, startCol);
            } else {
                errors_.error(filename_, startLine, startCol,
                              "Unexpected '|'. Use '||' for logical OR.");
            }
            break;

        case '@':
            advance();
            addToken(TokenType::OP_AT, "@", startLine, startCol);
            break;

        case '^':
            advance();
            addToken(TokenType::OP_DEREF, "^", startLine, startCol);
            break;

        case '{':
            advance();
            addToken(TokenType::PUNCT_LBRACE, "{", startLine, startCol);
            break;

        case '}':
            advance();
            addToken(TokenType::PUNCT_RBRACE, "}", startLine, startCol);
            break;

        case '(':
            advance();
            addToken(TokenType::PUNCT_LPAREN, "(", startLine, startCol);
            break;

        case ')':
            advance();
            addToken(TokenType::PUNCT_RPAREN, ")", startLine, startCol);
            break;

        case '[':
            advance();
            addToken(TokenType::PUNCT_LBRACKET, "[", startLine, startCol);
            break;

        case ']':
            advance();
            addToken(TokenType::PUNCT_RBRACKET, "]", startLine, startCol);
            break;

        case ';':
            advance();
            addToken(TokenType::PUNCT_SEMICOLON, ";", startLine, startCol);
            break;

        case ':':
            advance();
            addToken(TokenType::PUNCT_COLON, ":", startLine, startCol);
            break;

        case ',':
            advance();
            addToken(TokenType::PUNCT_COMMA, ",", startLine, startCol);
            break;

        case '.':
            advance();
            if (!isAtEnd() && current() == '.') {
                advance();
                addToken(TokenType::PUNCT_DOTDOT, "..", startLine, startCol);
            } else {
                addToken(TokenType::PUNCT_DOT, ".", startLine, startCol);
            }
            break;

        default:
            errors_.error(filename_, line_, column_,
                          std::string("Unexpected character: '") + c + "'.");
            advance();
            break;
    }
}

void Lexer::addToken(TokenType type, const std::string& lexeme, int startLine, int startCol) {
    tokens_.push_back(Token(type, lexeme, startLine, startCol));
}
