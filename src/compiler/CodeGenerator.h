#ifndef DICTATORSCRIPT_CODE_GENERATOR_H
#define DICTATORSCRIPT_CODE_GENERATOR_H

#include "ASTNode.h"
#include "ErrorReporter.h"
#include <string>
#include <sstream>
#include <vector>

// CodeGenerator walks the validated AST via the visitor pattern and emits
// clean, readable C++ source code.
//
// Expression visitors push their generated C++ string onto exprStack_.
// Statement visitors emit directly to the output_ stream.
//
// Extension point: to handle new node types, add a visit() override and
// update toCppType() / emitBuiltinMethod() as needed.
class CodeGenerator : public ASTVisitor {
public:
    CodeGenerator(ErrorReporter& errorReporter);

    // Generate C++ code from the AST. Returns the complete C++ source.
    std::string generate(ProgramNode& program);

    // Top-level
    void visit(ProgramNode& node) override;
    void visit(StructDefNode& node) override;
    void visit(FunctionDefNode& node) override;
    void visit(RegimeNode& node) override;
    void visit(EnlistNode& node) override;

    // Statements
    void visit(BlockNode& node) override;
    void visit(VarDeclNode& node) override;
    void visit(AssignStmtNode& node) override;
    void visit(IfStmtNode& node) override;
    void visit(ForRangeNode& node) override;
    void visit(ForEachNode& node) override;
    void visit(WhileNode& node) override;
    void visit(ForeverNode& node) override;
    void visit(BreakNode& node) override;
    void visit(ContinueNode& node) override;
    void visit(ReturnNode& node) override;
    void visit(BroadcastNode& node) override;
    void visit(DemandNode& node) override;
    void visit(KillNode& node) override;
    void visit(ExprStmtNode& node) override;
    void visit(UnpackStmtNode& node) override;

    // Expressions (push result onto exprStack_)
    void visit(BinaryExprNode& node) override;
    void visit(UnaryExprNode& node) override;
    void visit(IntLiteralNode& node) override;
    void visit(FloatLiteralNode& node) override;
    void visit(StringLiteralNode& node) override;
    void visit(CharLiteralNode& node) override;
    void visit(BoolLiteralNode& node) override;
    void visit(NullLiteralNode& node) override;
    void visit(IdentifierNode& node) override;
    void visit(FuncCallNode& node) override;
    void visit(MethodCallNode& node) override;
    void visit(MemberAccessNode& node) override;
    void visit(IndexAccessNode& node) override;
    void visit(ArrayLiteralNode& node) override;
    void visit(RangeLiteralNode& node) override;
    void visit(InitListNode& node) override;
    void visit(SummonExprNode& node) override;
    void visit(LambdaExprNode& node) override;
    void visit(SpawnExprNode& node) override;

private:
    ErrorReporter& errors_;
    std::ostringstream output_;
    int indentLevel_ = 0;

    // Stack for collecting generated expression strings.
    std::vector<std::string> exprStack_;

    // Emit helpers
    void emit(const std::string& text);
    void emitLine(const std::string& text);
    void emitIndent();
    void indent();
    void dedent();

    // Pop the most recent expression string from the stack.
    std::string popExpr();

    // Convert a TypeSpec to its C++ equivalent.
    std::string toCppType(const TypeSpec& type) const;

    // Get the zero-initialization value for a type.
    std::string getZeroValue(const TypeSpec& type) const;

    // Escape a string for C++ string literal output.
    std::string escapeString(const std::string& s) const;

    // Escape a char for C++ char literal output.
    std::string escapeChar(char c) const;

    // Convert operator token to C++ operator string.
    std::string operatorToString(TokenType op) const;

    // Flatten a + chain for broadcast output.
    void flattenPlusChain(ExprNode* expr, std::vector<ExprNode*>& parts);

    // Emit standard #include headers.
    void emitHeaders();

    // Emit forward declarations for structs and functions.
    void emitForwardDeclarations(ProgramNode& program);
};

#endif // DICTATORSCRIPT_CODE_GENERATOR_H
