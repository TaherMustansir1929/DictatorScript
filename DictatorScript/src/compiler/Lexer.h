#ifndef DICTATORSCRIPT_LEXER_H
#define DICTATORSCRIPT_LEXER_H

#include "Token.h"
#include "ErrorReporter.h"
#include <string>
#include <vector>
#include <unordered_map>

// Lexer reads raw DictatorScript source text and produces a flat stream of Tokens.
// Handles whitespace, comments, string/char literals, numeric literals,
// identifiers, keywords, operators, and punctuation.
//
// Extension point: to add a new keyword, add it to the keywords_ map in the
// constructor and add a corresponding TokenType entry.
class Lexer {
public:
    Lexer(const std::string& source, const std::string& filename,
          ErrorReporter& errorReporter);

    // Tokenize the entire source and return the token stream.
    std::vector<Token> tokenize();

private:
    std::string source_;
    std::string filename_;
    ErrorReporter& errors_;
    int pos_;
    int line_;
    int column_;
    std::vector<Token> tokens_;
    std::unordered_map<std::string, TokenType> keywords_;

    void initKeywords();

    // Character inspection helpers.
    char current() const;
    char peek() const;
    char peekAt(int offset) const;
    bool isAtEnd() const;
    char advance();

    // Skip whitespace (not newlines) and comments.
    void skipWhitespaceAndComments();
    void skipLineComment();
    bool skipBlockComment();

    // Token production methods.
    void scanToken();
    void scanString();
    void scanChar();
    void scanNumber();
    void scanIdentifierOrKeyword();
    void scanOperatorOrPunct();

    // Emit a token into the stream.
    void addToken(TokenType type, const std::string& lexeme, int startLine, int startCol);
};

#endif // DICTATORSCRIPT_LEXER_H
