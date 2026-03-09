#ifndef DICTATORSCRIPT_PARSER_H
#define DICTATORSCRIPT_PARSER_H

#include "Token.h"
#include "ASTNode.h"
#include "ErrorReporter.h"
#include <vector>
#include <memory>

// Recursive descent parser for DictatorScript.
// Consumes a token stream produced by the Lexer and builds an AST.
//
// Grammar overview (simplified):
//   program       → (structDef | funcDef | regime | import)* EOF
//   structDef     → "law" IDENT "{" field* "}"
//   funcDef       → "command" IDENT "(" params ")" "->" type block
//   regime        → "regime" "start" block
//   block         → "{" statement* "}"
//   statement     → varDecl | assignment | if | loop | break | continue
//                 | return | broadcast | demand | kill | exprStmt
//
// Extension point: to add a new statement type, add a case in parseStatement()
// and create a corresponding parseXxx() method.
class Parser {
public:
    Parser(const std::vector<Token>& tokens, const std::string& filename,
           ErrorReporter& errorReporter);

    // Parse the entire token stream and return the root ProgramNode.
    std::unique_ptr<ProgramNode> parse();

private:
    std::vector<Token> tokens_;
    std::string filename_;
    ErrorReporter& errors_;
    int pos_;

    // ---- Token navigation helpers ----
    const Token& current() const;
    const Token& peek() const;
    const Token& peekAt(int offset) const;
    bool isAtEnd() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    const Token& expect(TokenType type, const std::string& errorMsg);
    void skipNewlines();
    void expectStatementEnd();
    TokenType peekNonNewline() const;

    // Check if a token type can start a type specification.
    bool isTypeKeyword(TokenType type) const;
    // Check if the current position looks like the start of a type + identifier (a declaration).
    bool looksLikeDeclaration() const;

    // ---- Parsing methods for top-level constructs ----
    NodePtr parseTopLevel();
    std::unique_ptr<StructDefNode> parseStructDef();
    std::unique_ptr<FunctionDefNode> parseFunctionDef();
    std::unique_ptr<RegimeNode> parseRegime();
    std::unique_ptr<EnlistNode> parseEnlist();

    // ---- Type parsing ----
    TypeSpec parseType();
    TypeSpec parseBaseType();

    // ---- Statement parsing ----
    std::unique_ptr<BlockNode> parseBlock();
    NodePtr parseStatement();
    std::unique_ptr<VarDeclNode> parseVarDecl(bool requireDeclare = true);
    NodePtr parseExpressionOrAssignment();
    std::unique_ptr<IfStmtNode> parseIf();
    NodePtr parseImpose(); // returns ForRange, ForEach, While, or Forever
    std::unique_ptr<ReturnNode> parseReturn();
    std::unique_ptr<BroadcastNode> parseBroadcast();
    std::unique_ptr<DemandNode> parseDemand();
    std::unique_ptr<KillNode> parseKill();

    // ---- Expression parsing (precedence climbing) ----
    ExprPtr parseExpression();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseAddSub();
    ExprPtr parseMulDiv();
    ExprPtr parseUnary();
    ExprPtr parsePostfix();
    ExprPtr parsePrimary();

    // ---- Expression helpers ----
    ExprPtr parseArrayOrRangeLiteral();
    ExprPtr parseInitList();
    ExprPtr parseSummon();
    std::vector<ExprPtr> parseArgList();

    // ---- Error recovery ----
    void synchronize();
};

#endif // DICTATORSCRIPT_PARSER_H
