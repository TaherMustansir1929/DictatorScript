#include "Parser.h"
#include <iostream>
#include <stdexcept>

// ============================================================================
// Constructor
// ============================================================================

Parser::Parser(const std::vector<Token>& tokens, const std::string& filename,
               ErrorReporter& errorReporter)
    : tokens_(tokens), filename_(filename), errors_(errorReporter), pos_(0) {}

// ============================================================================
// Token navigation helpers
// ============================================================================

const Token& Parser::current() const {
    if (pos_ < (int)tokens_.size()) return tokens_[pos_];
    return tokens_.back(); // should be EOF
}

const Token& Parser::peek() const {
    return peekAt(1);
}

const Token& Parser::peekAt(int offset) const {
    int idx = pos_ + offset;
    if (idx >= (int)tokens_.size()) return tokens_.back();
    return tokens_[idx];
}

bool Parser::isAtEnd() const {
    return current().type == TokenType::TOKEN_EOF;
}

const Token& Parser::advance() {
    const Token& tok = current();
    if (!isAtEnd()) pos_++;
    return tok;
}

bool Parser::check(TokenType type) const {
    return current().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::expect(TokenType type, const std::string& errorMsg) {
    if (check(type)) {
        return advance();
    }
    errors_.error(filename_, current().line, current().col, errorMsg +
                  " (got '" + current().lexeme + "')");
    // Return current token but don't advance to avoid cascading errors.
    return current();
}

void Parser::skipNewlines() {
    while (check(TokenType::TOKEN_NEWLINE) || check(TokenType::PUNCT_SEMICOLON)) {
        advance();
    }
}

void Parser::expectStatementEnd() {
    if (check(TokenType::TOKEN_NEWLINE) || check(TokenType::PUNCT_SEMICOLON)) {
        advance();
        return;
    }
    if (check(TokenType::TOKEN_EOF) || check(TokenType::PUNCT_RBRACE)) {
        return; // end of file or block closes the statement
    }
    errors_.error(filename_, current().line, current().col,
                  "Expected end of statement (newline or ';').");
}

TokenType Parser::peekNonNewline() const {
    int i = pos_;
    while (i < (int)tokens_.size() && tokens_[i].type == TokenType::TOKEN_NEWLINE) {
        i++;
    }
    if (i < (int)tokens_.size()) return tokens_[i].type;
    return TokenType::TOKEN_EOF;
}

bool Parser::isTypeKeyword(TokenType type) const {
    return type == TokenType::KW_INT || type == TokenType::KW_FLOAT ||
           type == TokenType::KW_DOUBLE || type == TokenType::KW_CHAR ||
           type == TokenType::KW_STRING || type == TokenType::KW_BOOL ||
           type == TokenType::KW_VOID || type == TokenType::KW_MAP ||
           type == TokenType::KW_AUTO ||   // Feature 3: auto type deduction
           type == TokenType::KW_GUARD ||  // Feature 1: unique_ptr
           type == TokenType::KW_SHARE;    // Feature 1: shared_ptr
}

bool Parser::looksLikeDeclaration() const {
    // Checks if current position looks like: Type Identifier
    // Type can be a keyword type or an identifier (struct name)
    if (isTypeKeyword(current().type)) return true;
    // Struct type: IDENTIFIER followed by IDENTIFIER (e.g., Student s)
    if (current().type == TokenType::TOKEN_IDENTIFIER &&
        peek().type == TokenType::TOKEN_IDENTIFIER) {
        return true;
    }
    return false;
}

// ============================================================================
// Top-level parsing
// ============================================================================

std::unique_ptr<ProgramNode> Parser::parse() {
    auto program = std::make_unique<ProgramNode>();
    program->line = 1;
    program->col = 1;

    skipNewlines();
    while (!isAtEnd()) {
        auto node = parseTopLevel();
        if (node) {
            program->declarations.push_back(std::move(node));
        }
        skipNewlines();
    }

    return program;
}

NodePtr Parser::parseTopLevel() {
    skipNewlines();
    if (isAtEnd()) return nullptr;

    switch (current().type) {
        case TokenType::KW_LAW:
            return parseStructDef();
        case TokenType::KW_COMMAND:
            return parseFunctionDef();
        case TokenType::KW_REGIME:
            return parseRegime();
        case TokenType::KW_ENLIST:
            return parseEnlist();
        default:
            errors_.error(filename_, current().line, current().col,
                          "Expected top-level declaration (law, command, regime, or import).");
            synchronize();
            return nullptr;
    }
}

// ============================================================================
// Struct definition: law Name { field1\n field2\n ... }
// ============================================================================

std::unique_ptr<StructDefNode> Parser::parseStructDef() {
    auto node = std::make_unique<StructDefNode>();
    node->line = current().line;
    node->col = current().col;

    expect(TokenType::KW_LAW, "Expected 'law'");
    const Token& nameToken = expect(TokenType::TOKEN_IDENTIFIER, "Expected struct name after 'law'");
    node->name = nameToken.lexeme;

    skipNewlines();
    expect(TokenType::PUNCT_LBRACE, "Expected '{' after struct name");
    skipNewlines();

    while (!check(TokenType::PUNCT_RBRACE) && !isAtEnd()) {
        StructField field;
        field.line = current().line;
        field.col = current().col;
        field.type = parseType();
        const Token& fieldName = expect(TokenType::TOKEN_IDENTIFIER, "Expected field name");
        field.name = fieldName.lexeme;
        node->fields.push_back(std::move(field));
        skipNewlines();
    }

    expect(TokenType::PUNCT_RBRACE, "Expected '}' to close struct definition");
    return node;
}

// ============================================================================
// Function definition: command name(params) -> retType { body }
// ============================================================================

std::unique_ptr<FunctionDefNode> Parser::parseFunctionDef() {
    auto node = std::make_unique<FunctionDefNode>();
    node->line = current().line;
    node->col = current().col;

    expect(TokenType::KW_COMMAND, "Expected 'command'");
    const Token& nameToken = expect(TokenType::TOKEN_IDENTIFIER, "Expected function name after 'command'");
    node->name = nameToken.lexeme;

    expect(TokenType::PUNCT_LPAREN, "Expected '(' after function name");

    // Parse parameter list
    if (!check(TokenType::PUNCT_RPAREN)) {
        do {
            Parameter param;
            param.line = current().line;
            param.col = current().col;
            param.type = parseType();
            const Token& paramName = expect(TokenType::TOKEN_IDENTIFIER, "Expected parameter name");
            param.name = paramName.lexeme;
            node->params.push_back(std::move(param));
        } while (match(TokenType::PUNCT_COMMA));
    }

    expect(TokenType::PUNCT_RPAREN, "Expected ')' after parameters");
    expect(TokenType::OP_ARROW, "Expected '->' before return type");
    node->returnType = parseType();

    skipNewlines();
    node->body = parseBlock();

    return node;
}

// ============================================================================
// Regime: regime start { body }
// ============================================================================

std::unique_ptr<RegimeNode> Parser::parseRegime() {
    auto node = std::make_unique<RegimeNode>();
    node->line = current().line;
    node->col = current().col;

    expect(TokenType::KW_REGIME, "Expected 'regime'");

    // Expect "start" identifier
    if (check(TokenType::TOKEN_IDENTIFIER) && current().lexeme == "start") {
        advance();
    } else {
        errors_.error(filename_, current().line, current().col,
                      "Expected 'start' after 'regime'.");
    }

    skipNewlines();
    node->body = parseBlock();

    return node;
}

// ============================================================================
// Import: import "filename"
// ============================================================================

std::unique_ptr<EnlistNode> Parser::parseEnlist() {
    auto node = std::make_unique<EnlistNode>();
    node->line = current().line;
    node->col = current().col;

    expect(TokenType::KW_ENLIST, "Expected 'import'");
    const Token& fileToken = expect(TokenType::TOKEN_STRING_LITERAL, "Expected filename string after 'import'");
    node->filename = fileToken.lexeme;

    return node;
}

// ============================================================================
// Type parsing
// ============================================================================

TypeSpec Parser::parseType() {
    TypeSpec base = parseBaseType();

    // Check for array modifier: Type[]
    if (check(TokenType::PUNCT_LBRACKET) && peek().type == TokenType::PUNCT_RBRACKET) {
        advance(); // [
        advance(); // ]
        return TypeSpec::makeArray(base);
    }

    // Check for pointer modifier: Type->
    if (check(TokenType::OP_ARROW)) {
        advance();
        return TypeSpec::makePointer(base);
    }

    return base;
}

TypeSpec Parser::parseBaseType() {
    const Token& tok = current();

    switch (tok.type) {
        case TokenType::KW_INT:    advance(); return TypeSpec::makePrimitive("int");
        case TokenType::KW_FLOAT:  advance(); return TypeSpec::makePrimitive("float");
        case TokenType::KW_DOUBLE: advance(); return TypeSpec::makePrimitive("double");
        case TokenType::KW_CHAR:   advance(); return TypeSpec::makePrimitive("char");
        case TokenType::KW_STRING: advance(); return TypeSpec::makePrimitive("string");
        case TokenType::KW_BOOL:   advance(); return TypeSpec::makePrimitive("bool");
        case TokenType::KW_VOID:   advance(); return TypeSpec::makePrimitive("void");
        // Feature 3: auto type deduction
        case TokenType::KW_AUTO:   advance(); return TypeSpec(TypeSpec::AUTODEDUCE, "auto");
        // Feature 1: smart pointer types — guard T-> and share T->
        case TokenType::KW_GUARD: {
            advance(); // consume 'guard'
            TypeSpec inner = parseBaseType();
            TypeSpec t(TypeSpec::UNIQUE_PTR, inner.name);
            t.subType = std::make_shared<TypeSpec>(inner);
            return t;
        }
        case TokenType::KW_SHARE: {
            advance(); // consume 'share'
            TypeSpec inner = parseBaseType();
            TypeSpec t(TypeSpec::SHARED_PTR, inner.name);
            t.subType = std::make_shared<TypeSpec>(inner);
            return t;
        }
        case TokenType::KW_MAP: {
            advance(); // consume "map"
            expect(TokenType::OP_LT, "Expected '<' after 'map'");
            TypeSpec keyType = parseType();
            expect(TokenType::PUNCT_COMMA, "Expected ',' between map key and value types");
            TypeSpec valType = parseType();
            expect(TokenType::OP_GT, "Expected '>' to close map type");
            return TypeSpec::makeMap(keyType, valType);
        }
        case TokenType::TOKEN_IDENTIFIER: {
            std::string name = tok.lexeme;
            advance();
            return TypeSpec::makeStruct(name);
        }
        default:
            errors_.error(filename_, tok.line, tok.col,
                          "Expected type name, got '" + tok.lexeme + "'.");
            return TypeSpec::makeVoid();
    }
}

// ============================================================================
// Block parsing
// ============================================================================

std::unique_ptr<BlockNode> Parser::parseBlock() {
    auto block = std::make_unique<BlockNode>();
    block->line = current().line;
    block->col = current().col;

    expect(TokenType::PUNCT_LBRACE, "Expected '{'");
    skipNewlines();

    while (!check(TokenType::PUNCT_RBRACE) && !isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        }
        // Consume statement-ending newlines/semicolons
        if (!check(TokenType::PUNCT_RBRACE) && !isAtEnd()) {
            expectStatementEnd();
        }
        skipNewlines();
    }

    expect(TokenType::PUNCT_RBRACE, "Expected '}'");
    return block;
}

// ============================================================================
// Statement parsing
// ============================================================================

NodePtr Parser::parseStatement() {
    skipNewlines();
    if (isAtEnd()) return nullptr;

    switch (current().type) {
        case TokenType::KW_DECLARE:
            return parseVarDecl(true);

        case TokenType::KW_INTERROGATE:
            return parseIf();

        case TokenType::KW_IMPOSE:
            return parseImpose();

        case TokenType::KW_SILENCE: {
            auto node = std::make_unique<BreakNode>();
            node->line = current().line;
            node->col = current().col;
            advance();
            return node;
        }

        case TokenType::KW_PROCEED: {
            auto node = std::make_unique<ContinueNode>();
            node->line = current().line;
            node->col = current().col;
            advance();
            return node;
        }

        case TokenType::KW_REPORT:
            return parseReturn();

        case TokenType::KW_BROADCAST:
            return parseBroadcast();

        case TokenType::KW_DEMAND:
            return parseDemand();

        case TokenType::KW_KILL:
            return parseKill();

        // Feature 5: structured bindings — unpack [a,b] = expr
        case TokenType::KW_UNPACK:
            return parseUnpack();

        default:
            // Check if it looks like a declaration without 'declare' (e.g., Student s = {...})
            if (looksLikeDeclaration()) {
                return parseVarDecl(false);
            }
            return parseExpressionOrAssignment();
    }
}

// ============================================================================
// Variable declaration
// ============================================================================

std::unique_ptr<VarDeclNode> Parser::parseVarDecl(bool requireDeclare) {
    auto node = std::make_unique<VarDeclNode>();
    node->line = current().line;
    node->col = current().col;

    if (requireDeclare) {
        expect(TokenType::KW_DECLARE, "Expected 'declare'");
    }

    // Check for elite (const) modifier
    if (match(TokenType::KW_ELITE)) {
        node->isConst = true;
    }

    node->type = parseType();

    const Token& nameToken = expect(TokenType::TOKEN_IDENTIFIER, "Expected variable name");
    node->name = nameToken.lexeme;

    // Optional initializer
    if (match(TokenType::OP_ASSIGN)) {
        node->initializer = parseExpression();
    }

    return node;
}

// ============================================================================
// Expression statement or assignment
// ============================================================================

NodePtr Parser::parseExpressionOrAssignment() {
    int startLine = current().line;
    int startCol = current().col;

    ExprPtr expr = parseExpression();

    // Check for assignment operators
    TokenType assignOp = current().type;
    if (assignOp == TokenType::OP_ASSIGN || assignOp == TokenType::OP_PLUS_ASSIGN ||
        assignOp == TokenType::OP_MINUS_ASSIGN || assignOp == TokenType::OP_STAR_ASSIGN ||
        assignOp == TokenType::OP_SLASH_ASSIGN) {
        advance(); // consume assignment operator
        auto assignNode = std::make_unique<AssignStmtNode>();
        assignNode->line = startLine;
        assignNode->col = startCol;
        assignNode->target = std::move(expr);
        assignNode->op = assignOp;
        assignNode->value = parseExpression();
        return assignNode;
    }

    // It's an expression statement (e.g., function call)
    auto exprStmt = std::make_unique<ExprStmtNode>();
    exprStmt->line = startLine;
    exprStmt->col = startCol;
    exprStmt->expression = std::move(expr);
    return exprStmt;
}

// ============================================================================
// If statement: interrogate (cond) { ... } [otherwise [interrogate (cond)] { ... }]
// ============================================================================

std::unique_ptr<IfStmtNode> Parser::parseIf() {
    auto node = std::make_unique<IfStmtNode>();
    node->line = current().line;
    node->col = current().col;

    expect(TokenType::KW_INTERROGATE, "Expected 'interrogate'");
    expect(TokenType::PUNCT_LPAREN, "Expected '(' after 'interrogate'");
    node->condition = parseExpression();
    expect(TokenType::PUNCT_RPAREN, "Expected ')' after condition");

    skipNewlines();
    node->thenBlock = parseBlock();

    // Check for otherwise (else / else if)
    // Look past newlines for 'otherwise'
    int saved = pos_;
    skipNewlines();
    if (check(TokenType::KW_OTHERWISE)) {
        advance(); // consume 'otherwise'

        skipNewlines();
        if (check(TokenType::KW_INTERROGATE)) {
            // otherwise interrogate → else if
            node->elseBlock = parseIf();
        } else {
            // otherwise → else
            node->elseBlock = parseBlock();
        }
    } else {
        // No else clause — restore position (don't consume newlines)
        pos_ = saved;
    }

    return node;
}

// ============================================================================
// Loop parsing: impose (various forms)
// ============================================================================

NodePtr Parser::parseImpose() {
    int startLine = current().line;
    int startCol = current().col;

    expect(TokenType::KW_IMPOSE, "Expected 'impose'");
    skipNewlines();

    // Check for "impose forever"
    if (check(TokenType::KW_FOREVER)) {
        advance();
        auto node = std::make_unique<ForeverNode>();
        node->line = startLine;
        node->col = startCol;
        skipNewlines();
        node->body = parseBlock();
        return node;
    }

    expect(TokenType::PUNCT_LPAREN, "Expected '(' or 'forever' after 'impose'");

    // Determine which loop variant:
    // 1. Type Identifier KW_FROM → for-range or for-each
    // 2. Otherwise → while loop

    bool isForLoop = false;
    if (isTypeKeyword(current().type)) {
        // Starts with a type keyword — it's a for loop
        isForLoop = true;
    } else if (current().type == TokenType::TOKEN_IDENTIFIER) {
        // Could be a struct type (e.g., Student s from students)
        // or a while condition (e.g., flag, a < b)
        // Check if next token is also an identifier (indicating Type Name pattern)
        if (peek().type == TokenType::TOKEN_IDENTIFIER) {
            isForLoop = true;
        }
    }

    if (isForLoop) {
        TypeSpec varType = parseType();
        const Token& varNameToken = expect(TokenType::TOKEN_IDENTIFIER, "Expected loop variable name");
        std::string varName = varNameToken.lexeme;

        expect(TokenType::KW_FROM, "Expected 'from' in impose loop");

        // Check if it's a range [a..b] or a collection
        if (check(TokenType::PUNCT_LBRACKET)) {
            advance(); // consume [
            ExprPtr rangeStart = parseExpression();

            if (match(TokenType::PUNCT_DOTDOT)) {
                ExprPtr rangeEnd = parseExpression();
                expect(TokenType::PUNCT_RBRACKET, "Expected ']' to close range");
                expect(TokenType::PUNCT_RPAREN, "Expected ')' after range");

                auto node = std::make_unique<ForRangeNode>();
                node->line = startLine;
                node->col = startCol;
                node->varType = varType;
                node->varName = varName;
                node->start = std::move(rangeStart);
                node->end = std::move(rangeEnd);
                skipNewlines();
                node->body = parseBlock();
                return node;
            } else {
                errors_.error(filename_, current().line, current().col,
                              "Expected '..' in range literal.");
                return nullptr;
            }
        } else {
            // For-each: collection is an expression
            ExprPtr collection = parseExpression();
            expect(TokenType::PUNCT_RPAREN, "Expected ')' after collection");

            auto node = std::make_unique<ForEachNode>();
            node->line = startLine;
            node->col = startCol;
            node->varType = varType;
            node->varName = varName;
            node->collection = std::move(collection);
            skipNewlines();
            node->body = parseBlock();
            return node;
        }
    } else {
        // While loop: impose (condition) { body }
        ExprPtr condition = parseExpression();
        expect(TokenType::PUNCT_RPAREN, "Expected ')' after condition");

        auto node = std::make_unique<WhileNode>();
        node->line = startLine;
        node->col = startCol;
        node->condition = std::move(condition);
        skipNewlines();
        node->body = parseBlock();
        return node;
    }
}

// ============================================================================
// Return: report [expr]
// ============================================================================

std::unique_ptr<ReturnNode> Parser::parseReturn() {
    auto node = std::make_unique<ReturnNode>();
    node->line = current().line;
    node->col = current().col;

    expect(TokenType::KW_REPORT, "Expected 'report'");

    // Check if there's an expression to return
    if (!check(TokenType::TOKEN_NEWLINE) && !check(TokenType::PUNCT_SEMICOLON) &&
        !check(TokenType::PUNCT_RBRACE) && !isAtEnd()) {
        node->value = parseExpression();
    }

    return node;
}

// ============================================================================
// Broadcast: broadcast(expr)
// ============================================================================

std::unique_ptr<BroadcastNode> Parser::parseBroadcast() {
    auto node = std::make_unique<BroadcastNode>();
    node->line = current().line;
    node->col = current().col;

    expect(TokenType::KW_BROADCAST, "Expected 'broadcast'");
    expect(TokenType::PUNCT_LPAREN, "Expected '(' after 'broadcast'");
    node->expression = parseExpression();
    expect(TokenType::PUNCT_RPAREN, "Expected ')' after broadcast expression");

    return node;
}

// ============================================================================
// Demand: demand(target)
// ============================================================================

std::unique_ptr<DemandNode> Parser::parseDemand() {
    auto node = std::make_unique<DemandNode>();
    node->line = current().line;
    node->col = current().col;

    expect(TokenType::KW_DEMAND, "Expected 'demand'");
    expect(TokenType::PUNCT_LPAREN, "Expected '(' after 'demand'");
    node->target = parseExpression();
    expect(TokenType::PUNCT_RPAREN, "Expected ')' after demand variable");

    return node;
}

// ============================================================================
// Kill: kill expr
// ============================================================================

std::unique_ptr<KillNode> Parser::parseKill() {
    auto node = std::make_unique<KillNode>();
    node->line = current().line;
    node->col = current().col;

    expect(TokenType::KW_KILL, "Expected 'kill'");
    node->target = parseExpression();

    return node;
}

// ============================================================================
// Feature 5: Structured bindings — unpack [a, b, ...] = expr
// ============================================================================

std::unique_ptr<UnpackStmtNode> Parser::parseUnpack() {
    auto node = std::make_unique<UnpackStmtNode>();
    node->line = current().line;
    node->col  = current().col;

    expect(TokenType::KW_UNPACK, "Expected 'unpack'");
    expect(TokenType::PUNCT_LBRACKET, "Expected '[' after 'unpack'");

    // Parse binding names: [a, b, c, ...]
    do {
        const Token& nameToken = expect(TokenType::TOKEN_IDENTIFIER,
                                        "Expected identifier in unpack binding list");
        node->bindings.push_back(nameToken.lexeme);
    } while (match(TokenType::PUNCT_COMMA));

    expect(TokenType::PUNCT_RBRACKET, "Expected ']' to close binding list");
    expect(TokenType::OP_ASSIGN, "Expected '=' after unpack binding list");
    node->expression = parseExpression();

    return node;
}

// ============================================================================
// Expression parsing — precedence climbing
// ============================================================================

ExprPtr Parser::parseExpression() {
    return parseOr();
}

// Helper: parse a comma-separated list of expressions enclosed in '(' ... ')'
// Leaves the parser positioned after the closing ')'.
std::vector<ExprPtr> Parser::parseArgList() {
    std::vector<ExprPtr> args;
    expect(TokenType::PUNCT_LPAREN, "Expected '(' to begin argument list");
    if (!check(TokenType::PUNCT_RPAREN)) {
        do {
            args.push_back(parseExpression());
        } while (match(TokenType::PUNCT_COMMA));
    }
    expect(TokenType::PUNCT_RPAREN, "Expected ')' to close argument list");
    return args;
}


ExprPtr Parser::parseOr() {
    ExprPtr left = parseAnd();

    while (check(TokenType::OP_OR)) {
        TokenType op = current().type;
        int opLine = current().line;
        int opCol = current().col;
        advance();
        ExprPtr right = parseAnd();

        auto node = std::make_unique<BinaryExprNode>();
        node->line = opLine;
        node->col = opCol;
        node->left = std::move(left);
        node->op = op;
        node->right = std::move(right);
        left = std::move(node);
    }

    return left;
}

ExprPtr Parser::parseAnd() {
    ExprPtr left = parseEquality();

    while (check(TokenType::OP_AND)) {
        TokenType op = current().type;
        int opLine = current().line;
        int opCol = current().col;
        advance();
        ExprPtr right = parseEquality();

        auto node = std::make_unique<BinaryExprNode>();
        node->line = opLine;
        node->col = opCol;
        node->left = std::move(left);
        node->op = op;
        node->right = std::move(right);
        left = std::move(node);
    }

    return left;
}

ExprPtr Parser::parseEquality() {
    ExprPtr left = parseComparison();

    while (check(TokenType::OP_EQ) || check(TokenType::OP_NEQ)) {
        TokenType op = current().type;
        int opLine = current().line;
        int opCol = current().col;
        advance();
        ExprPtr right = parseComparison();

        auto node = std::make_unique<BinaryExprNode>();
        node->line = opLine;
        node->col = opCol;
        node->left = std::move(left);
        node->op = op;
        node->right = std::move(right);
        left = std::move(node);
    }

    return left;
}

ExprPtr Parser::parseComparison() {
    ExprPtr left = parseAddSub();

    while (check(TokenType::OP_LT) || check(TokenType::OP_GT) ||
           check(TokenType::OP_LTE) || check(TokenType::OP_GTE)) {
        TokenType op = current().type;
        int opLine = current().line;
        int opCol = current().col;
        advance();
        ExprPtr right = parseAddSub();

        auto node = std::make_unique<BinaryExprNode>();
        node->line = opLine;
        node->col = opCol;
        node->left = std::move(left);
        node->op = op;
        node->right = std::move(right);
        left = std::move(node);
    }

    return left;
}

ExprPtr Parser::parseAddSub() {
    ExprPtr left = parseMulDiv();

    while (check(TokenType::OP_PLUS) || check(TokenType::OP_MINUS)) {
        TokenType op = current().type;
        int opLine = current().line;
        int opCol = current().col;
        advance();
        ExprPtr right = parseMulDiv();

        auto node = std::make_unique<BinaryExprNode>();
        node->line = opLine;
        node->col = opCol;
        node->left = std::move(left);
        node->op = op;
        node->right = std::move(right);
        left = std::move(node);
    }

    return left;
}

ExprPtr Parser::parseMulDiv() {
    ExprPtr left = parseUnary();

    while (check(TokenType::OP_STAR) || check(TokenType::OP_SLASH) ||
           check(TokenType::OP_PERCENT)) {
        TokenType op = current().type;
        int opLine = current().line;
        int opCol = current().col;
        advance();
        ExprPtr right = parseUnary();

        auto node = std::make_unique<BinaryExprNode>();
        node->line = opLine;
        node->col = opCol;
        node->left = std::move(left);
        node->op = op;
        node->right = std::move(right);
        left = std::move(node);
    }

    return left;
}

ExprPtr Parser::parseUnary() {
    if (check(TokenType::OP_MINUS) || check(TokenType::OP_NOT) ||
        check(TokenType::OP_AT) || check(TokenType::OP_DEREF)) {
        TokenType op = current().type;
        int opLine = current().line;
        int opCol = current().col;
        advance();

        auto node = std::make_unique<UnaryExprNode>();
        node->line = opLine;
        node->col = opCol;
        node->op = op;
        node->operand = parseUnary(); // right-recursive for prefix
        return node;
    }

    return parsePostfix();
}

ExprPtr Parser::parsePostfix() {
    ExprPtr expr = parsePrimary();

    // Handle postfix operations: .member, .method(), [index]
    while (true) {
        if (check(TokenType::PUNCT_DOT)) {
            advance(); // consume .
            const Token& memberToken = expect(TokenType::TOKEN_IDENTIFIER, "Expected member name after '.'");

            // Check if it's a method call: .name(args)
            if (check(TokenType::PUNCT_LPAREN)) {
                advance(); // consume (
                auto node = std::make_unique<MethodCallNode>();
                node->line = memberToken.line;
                node->col = memberToken.col;
                node->object = std::move(expr);
                node->methodName = memberToken.lexeme;

                // Parse arguments
                if (!check(TokenType::PUNCT_RPAREN)) {
                    do {
                        node->arguments.push_back(parseExpression());
                    } while (match(TokenType::PUNCT_COMMA));
                }
                expect(TokenType::PUNCT_RPAREN, "Expected ')' after method arguments");
                expr = std::move(node);
            } else {
                // Member access: .name
                auto node = std::make_unique<MemberAccessNode>();
                node->line = memberToken.line;
                node->col = memberToken.col;
                node->object = std::move(expr);
                node->fieldName = memberToken.lexeme;
                expr = std::move(node);
            }
        } else if (check(TokenType::PUNCT_LBRACKET)) {
            int bracketLine = current().line;
            int bracketCol = current().col;
            advance(); // consume [
            ExprPtr index = parseExpression();
            expect(TokenType::PUNCT_RBRACKET, "Expected ']' after index");

            auto node = std::make_unique<IndexAccessNode>();
            node->line = bracketLine;
            node->col = bracketCol;
            node->object = std::move(expr);
            node->index = std::move(index);
            expr = std::move(node);
        } else {
            break;
        }
    }

    return expr;
}

ExprPtr Parser::parsePrimary() {
    const Token& tok = current();

    switch (tok.type) {
        case TokenType::TOKEN_INT_LITERAL: {
            advance();
            auto node = std::make_unique<IntLiteralNode>();
            node->line = tok.line;
            node->col = tok.col;
            node->value = std::stoll(tok.lexeme);
            return node;
        }

        case TokenType::TOKEN_FLOAT_LITERAL: {
            advance();
            auto node = std::make_unique<FloatLiteralNode>();
            node->line = tok.line;
            node->col = tok.col;
            node->value = std::stod(tok.lexeme);
            return node;
        }

        case TokenType::TOKEN_STRING_LITERAL: {
            advance();
            auto node = std::make_unique<StringLiteralNode>();
            node->line = tok.line;
            node->col = tok.col;
            node->value = tok.lexeme;
            return node;
        }

        case TokenType::TOKEN_CHAR_LITERAL: {
            advance();
            auto node = std::make_unique<CharLiteralNode>();
            node->line = tok.line;
            node->col = tok.col;
            node->value = tok.lexeme.empty() ? '\0' : tok.lexeme[0];
            return node;
        }

        case TokenType::KW_LOYAL: {
            advance();
            auto node = std::make_unique<BoolLiteralNode>();
            node->line = tok.line;
            node->col = tok.col;
            node->value = true;
            return node;
        }

        case TokenType::KW_TRAITOR: {
            advance();
            auto node = std::make_unique<BoolLiteralNode>();
            node->line = tok.line;
            node->col = tok.col;
            node->value = false;
            return node;
        }

        case TokenType::KW_NULL_VAL: {
            advance();
            auto node = std::make_unique<NullLiteralNode>();
            node->line = tok.line;
            node->col = tok.col;
            return node;
        }

        case TokenType::TOKEN_IDENTIFIER: {
            std::string name = tok.lexeme;
            int identLine = tok.line;
            int identCol = tok.col;
            advance();

            // Check if it's a function call: name(args)
            if (check(TokenType::PUNCT_LPAREN)) {
                advance(); // consume (
                auto node = std::make_unique<FuncCallNode>();
                node->line = identLine;
                node->col = identCol;
                node->name = name;

                if (!check(TokenType::PUNCT_RPAREN)) {
                    do {
                        node->arguments.push_back(parseExpression());
                    } while (match(TokenType::PUNCT_COMMA));
                }
                expect(TokenType::PUNCT_RPAREN, "Expected ')' after function arguments");
                return node;
            }

            // Just an identifier
            auto node = std::make_unique<IdentifierNode>();
            node->line = identLine;
            node->col = identCol;
            node->name = name;
            return node;
        }

        case TokenType::PUNCT_LPAREN: {
            advance(); // consume (
            ExprPtr expr = parseExpression();
            expect(TokenType::PUNCT_RPAREN, "Expected ')' after expression");
            return expr;
        }

        case TokenType::PUNCT_LBRACKET:
            return parseArrayOrRangeLiteral();

        case TokenType::PUNCT_LBRACE:
            return parseInitList();

        case TokenType::KW_SUMMON:
            return parseSummon();

        // Feature 1: Smart Pointer heap allocation — pass kind so codegen emits make_unique/make_shared
        case TokenType::KW_SUMMON_GUARD:
            return parseSummon(SummonExprNode::UNIQUE);
        case TokenType::KW_SUMMON_SHARE:
            return parseSummon(SummonExprNode::SHARED);

        // Feature 2: Lambda expression — block(params) -> retType { body }
        case TokenType::KW_BLOCK:
            return parseLambda();

        // Feature 4: Concurrency — spawn funcName(args)
        case TokenType::KW_SPAWN:
            return parseSpawn();

        case TokenType::OP_MINUS: {
            // Negative number or unary minus — handled in parseUnary
            // If we got here, it's an unexpected minus in primary position
            advance();
            auto node = std::make_unique<UnaryExprNode>();
            node->line = tok.line;
            node->col = tok.col;
            node->op = TokenType::OP_MINUS;
            node->operand = parsePrimary();
            return node;
        }

        default:
            errors_.error(filename_, tok.line, tok.col,
                          "Unexpected token in expression: '" + tok.lexeme + "'.");
            advance(); // skip the bad token
            // Return a dummy node
            auto dummy = std::make_unique<IntLiteralNode>();
            dummy->line = tok.line;
            dummy->col = tok.col;
            dummy->value = 0;
            return dummy;
    }
}

// ============================================================================
// Array or range literal: [expr, expr, ...] or [start..end]
// ============================================================================

ExprPtr Parser::parseArrayOrRangeLiteral() {
    int startLine = current().line;
    int startCol = current().col;
    advance(); // consume [

    if (check(TokenType::PUNCT_RBRACKET)) {
        // Empty array literal
        advance();
        auto node = std::make_unique<ArrayLiteralNode>();
        node->line = startLine;
        node->col = startCol;
        return node;
    }

    ExprPtr first = parseExpression();

    // Check for range: [start..end]
    if (check(TokenType::PUNCT_DOTDOT)) {
        advance(); // consume ..
        ExprPtr endExpr = parseExpression();
        expect(TokenType::PUNCT_RBRACKET, "Expected ']' to close range literal");

        auto node = std::make_unique<RangeLiteralNode>();
        node->line = startLine;
        node->col = startCol;
        node->start = std::move(first);
        node->end = std::move(endExpr);
        return node;
    }

    // Array literal: [expr, expr, ...]
    auto node = std::make_unique<ArrayLiteralNode>();
    node->line = startLine;
    node->col = startCol;
    node->elements.push_back(std::move(first));

    while (match(TokenType::PUNCT_COMMA)) {
        node->elements.push_back(parseExpression());
    }

    expect(TokenType::PUNCT_RBRACKET, "Expected ']' to close array literal");
    return node;
}

// ============================================================================
// Initializer list: {expr, expr, ...}
// ============================================================================

ExprPtr Parser::parseInitList() {
    int startLine = current().line;
    int startCol = current().col;
    advance(); // consume {

    auto node = std::make_unique<InitListNode>();
    node->line = startLine;
    node->col = startCol;

    if (!check(TokenType::PUNCT_RBRACE)) {
        do {
            node->values.push_back(parseExpression());
        } while (match(TokenType::PUNCT_COMMA));
    }

    expect(TokenType::PUNCT_RBRACE, "Expected '}' to close initializer list");
    return node;
}

// ============================================================================
// Summon expression: summon Type[(args)]
// ============================================================================

ExprPtr Parser::parseSummon(SummonExprNode::SummonKind kind) {
    int startLine = current().line;
    int startCol = current().col;
    advance(); // consume 'summon' / 'summon_guard' / 'summon_share'

    auto node = std::make_unique<SummonExprNode>();
    node->line = startLine;
    node->col = startCol;
    node->kind = kind;          // record which keyword was used
    node->type = parseBaseType();

    // Optional constructor arguments
    if (check(TokenType::PUNCT_LPAREN)) {
        advance();
        if (!check(TokenType::PUNCT_RPAREN)) {
            do {
                node->arguments.push_back(parseExpression());
            } while (match(TokenType::PUNCT_COMMA));
        }
        expect(TokenType::PUNCT_RPAREN, "Expected ')' after summon arguments");
    }

    return node;
}

// ============================================================================
// Feature 2: Lambda expression — block(params) -> retType { body }
// ============================================================================

ExprPtr Parser::parseLambda() {
    int startLine = current().line;
    int startCol  = current().col;
    advance(); // consume 'block'

    auto node = std::make_unique<LambdaExprNode>();
    node->line = startLine;
    node->col  = startCol;

    expect(TokenType::PUNCT_LPAREN, "Expected '(' after 'block'");

    // Parse parameter list (same as command)
    if (!check(TokenType::PUNCT_RPAREN)) {
        do {
            Parameter param;
            param.line = current().line;
            param.col  = current().col;
            param.type = parseType();
            const Token& paramName = expect(TokenType::TOKEN_IDENTIFIER, "Expected parameter name");
            param.name = paramName.lexeme;
            node->params.push_back(std::move(param));
        } while (match(TokenType::PUNCT_COMMA));
    }
    expect(TokenType::PUNCT_RPAREN, "Expected ')' after lambda parameters");
    expect(TokenType::OP_ARROW, "Expected '->' before lambda return type");
    node->returnType = parseType();

    skipNewlines();
    node->body = parseBlock();
    return node;
}

// ============================================================================
// Feature 4: Spawn expression — spawn funcName(args)
// ============================================================================

ExprPtr Parser::parseSpawn() {
    int startLine = current().line;
    int startCol  = current().col;
    advance(); // consume 'spawn'

    auto node = std::make_unique<SpawnExprNode>();
    node->line = startLine;
    node->col  = startCol;

    // Expect a function name
    const Token& nameToken = expect(TokenType::TOKEN_IDENTIFIER,
                                    "Expected function name after 'spawn'");
    node->funcName = nameToken.lexeme;

    // Optional arguments
    if (check(TokenType::PUNCT_LPAREN)) {
        advance();
        if (!check(TokenType::PUNCT_RPAREN)) {
            do {
                node->arguments.push_back(parseExpression());
            } while (match(TokenType::PUNCT_COMMA));
        }
        expect(TokenType::PUNCT_RPAREN, "Expected ')' after spawn arguments");
    }
    return node;
}

// ============================================================================
// Error recovery — skip to next statement boundary
// ============================================================================

void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (current().type == TokenType::TOKEN_NEWLINE ||
            current().type == TokenType::PUNCT_SEMICOLON) {
            advance();
            return;
        }
        if (current().type == TokenType::KW_LAW ||
            current().type == TokenType::KW_COMMAND ||
            current().type == TokenType::KW_REGIME ||
            current().type == TokenType::KW_DECLARE ||
            current().type == TokenType::KW_INTERROGATE ||
            current().type == TokenType::KW_IMPOSE ||
            current().type == TokenType::KW_REPORT ||
            current().type == TokenType::KW_BROADCAST ||
            current().type == TokenType::KW_DEMAND ||
            current().type == TokenType::KW_UNPACK) {
            return;
        }
        advance();
    }
}
