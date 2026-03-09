#ifndef DICTATORSCRIPT_SEMANTIC_ANALYZER_H
#define DICTATORSCRIPT_SEMANTIC_ANALYZER_H

#include "ASTNode.h"
#include "SymbolTable.h"
#include "ErrorReporter.h"
#include <string>
#include <optional>

// SemanticAnalyzer walks the AST using the visitor pattern and performs:
//   - Symbol table management (scoped)
//   - Type checking (basic)
//   - Undeclared variable detection
//   - Function signature validation
//   - Return type checking
//   - Duplicate declaration checking
//   - Const (elite) mutation checks
//
// Extension point: to add checks for a new node type, add a visit() override.
class SemanticAnalyzer : public ASTVisitor {
public:
    SemanticAnalyzer(const std::string& filename, ErrorReporter& errorReporter);

    // Run the analysis on the full AST.
    void analyze(ProgramNode& program);

    // Provide access to the symbol table for other stages (e.g., CodeGenerator).
    const SymbolTable& getSymbolTable() const { return symbols_; }

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

    // Expressions
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

private:
    std::string filename_;
    ErrorReporter& errors_;
    SymbolTable symbols_;

    // Track the current function's return type for return-type checking.
    std::optional<TypeSpec> currentReturnType_;

    // Track loop depth for break/continue validation.
    int loopDepth_ = 0;

    // Helper to resolve the type of an expression (after visiting).
    std::optional<TypeSpec> resolveExprType(ExprNode& expr);

    // Register all struct types in a first pass.
    void registerStructs(ProgramNode& program);

    // Register all function signatures in a first pass.
    void registerFunctions(ProgramNode& program);
};

#endif // DICTATORSCRIPT_SEMANTIC_ANALYZER_H
