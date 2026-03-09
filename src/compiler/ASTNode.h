#ifndef DICTATORSCRIPT_AST_NODE_H
#define DICTATORSCRIPT_AST_NODE_H

#include "TokenType.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>

// ============================================================================
// Forward declarations of all AST node types.
// Extension point: add new node classes here and add a visit() method to ASTVisitor.
// ============================================================================

class ProgramNode;
class StructDefNode;
class FunctionDefNode;
class RegimeNode;
class EnlistNode;
class BlockNode;
class VarDeclNode;
class AssignStmtNode;
class IfStmtNode;
class ForRangeNode;
class ForEachNode;
class WhileNode;
class ForeverNode;
class BreakNode;
class ContinueNode;
class ReturnNode;
class BroadcastNode;
class DemandNode;
class KillNode;
class ExprStmtNode;

// Expression nodes
class BinaryExprNode;
class UnaryExprNode;
class IntLiteralNode;
class FloatLiteralNode;
class StringLiteralNode;
class CharLiteralNode;
class BoolLiteralNode;
class NullLiteralNode;
class IdentifierNode;
class FuncCallNode;
class MethodCallNode;
class MemberAccessNode;
class IndexAccessNode;
class ArrayLiteralNode;
class RangeLiteralNode;
class InitListNode;
class SummonExprNode;

// ============================================================================
// TypeSpec — describes a DictatorScript type (primitive, array, pointer, map, struct).
// ============================================================================

struct TypeSpec {
    enum Kind { PRIMITIVE, ARRAY, POINTER, MAP, STRUCT };

    Kind kind;
    std::string name; // base type name: "int", "float", "Student", etc.
    std::shared_ptr<TypeSpec> subType;  // element type for ARRAY, pointee for POINTER
    std::shared_ptr<TypeSpec> keyType;  // key type for MAP
    std::shared_ptr<TypeSpec> valType;  // value type for MAP

    TypeSpec() : kind(PRIMITIVE), name("void") {}
    TypeSpec(Kind k, const std::string& n) : kind(k), name(n) {}

    // Human-readable type string for error messages and code generation.
    std::string toString() const;

    bool operator==(const TypeSpec& other) const;
    bool operator!=(const TypeSpec& other) const { return !(*this == other); }

    // Factory helpers
    static TypeSpec makePrimitive(const std::string& name);
    static TypeSpec makeArray(const TypeSpec& elemType);
    static TypeSpec makePointer(const TypeSpec& pointeeType);
    static TypeSpec makeMap(const TypeSpec& key, const TypeSpec& val);
    static TypeSpec makeStruct(const std::string& name);
    static TypeSpec makeVoid();
};

// ============================================================================
// ASTVisitor — visitor interface for traversing the AST.
// Extension point: add a new virtual visit() method for each new node type.
// ============================================================================

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // Top-level
    virtual void visit(ProgramNode& node) = 0;
    virtual void visit(StructDefNode& node) = 0;
    virtual void visit(FunctionDefNode& node) = 0;
    virtual void visit(RegimeNode& node) = 0;
    virtual void visit(EnlistNode& node) = 0;

    // Statements
    virtual void visit(BlockNode& node) = 0;
    virtual void visit(VarDeclNode& node) = 0;
    virtual void visit(AssignStmtNode& node) = 0;
    virtual void visit(IfStmtNode& node) = 0;
    virtual void visit(ForRangeNode& node) = 0;
    virtual void visit(ForEachNode& node) = 0;
    virtual void visit(WhileNode& node) = 0;
    virtual void visit(ForeverNode& node) = 0;
    virtual void visit(BreakNode& node) = 0;
    virtual void visit(ContinueNode& node) = 0;
    virtual void visit(ReturnNode& node) = 0;
    virtual void visit(BroadcastNode& node) = 0;
    virtual void visit(DemandNode& node) = 0;
    virtual void visit(KillNode& node) = 0;
    virtual void visit(ExprStmtNode& node) = 0;

    // Expressions
    virtual void visit(BinaryExprNode& node) = 0;
    virtual void visit(UnaryExprNode& node) = 0;
    virtual void visit(IntLiteralNode& node) = 0;
    virtual void visit(FloatLiteralNode& node) = 0;
    virtual void visit(StringLiteralNode& node) = 0;
    virtual void visit(CharLiteralNode& node) = 0;
    virtual void visit(BoolLiteralNode& node) = 0;
    virtual void visit(NullLiteralNode& node) = 0;
    virtual void visit(IdentifierNode& node) = 0;
    virtual void visit(FuncCallNode& node) = 0;
    virtual void visit(MethodCallNode& node) = 0;
    virtual void visit(MemberAccessNode& node) = 0;
    virtual void visit(IndexAccessNode& node) = 0;
    virtual void visit(ArrayLiteralNode& node) = 0;
    virtual void visit(RangeLiteralNode& node) = 0;
    virtual void visit(InitListNode& node) = 0;
    virtual void visit(SummonExprNode& node) = 0;
};

// ============================================================================
// ASTNode — abstract base class for all AST nodes.
// ============================================================================

class ASTNode {
public:
    int line = 0;
    int col = 0;

    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
    virtual void print(int indent = 0) const = 0;

protected:
    // Helper to print indentation.
    static void printIndent(int indent);
};

// ============================================================================
// ExprNode — base class for all expression nodes.
// Provides a resolvedType field set by the SemanticAnalyzer.
// ============================================================================

class ExprNode : public ASTNode {
public:
    std::optional<TypeSpec> resolvedType;
};

// Convenience aliases
using NodePtr = std::unique_ptr<ASTNode>;
using ExprPtr = std::unique_ptr<ExprNode>;

// ============================================================================
// Struct field — name + type pair used in StructDefNode
// ============================================================================

struct StructField {
    TypeSpec type;
    std::string name;
    int line = 0;
    int col = 0;
};

// Function parameter — type + name pair
struct Parameter {
    TypeSpec type;
    std::string name;
    int line = 0;
    int col = 0;
};

// ============================================================================
// TOP-LEVEL NODES
// ============================================================================

// Root of the AST — contains all top-level declarations.
class ProgramNode : public ASTNode {
public:
    std::vector<NodePtr> declarations;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// struct definition: law Name { fields }
class StructDefNode : public ASTNode {
public:
    std::string name;
    std::vector<StructField> fields;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Function definition: command name(params) -> retType { body }
class FunctionDefNode : public ASTNode {
public:
    std::string name;
    std::vector<Parameter> params;
    TypeSpec returnType;
    std::unique_ptr<BlockNode> body;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Entry point: regime start { body }
class RegimeNode : public ASTNode {
public:
    std::unique_ptr<BlockNode> body;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// File import: import "filename"
class EnlistNode : public ASTNode {
public:
    std::string filename;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// ============================================================================
// STATEMENT NODES
// ============================================================================

// Block of statements: { statements }
class BlockNode : public ASTNode {
public:
    std::vector<NodePtr> statements;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Variable declaration: declare [elite] Type name [= initializer]
class VarDeclNode : public ASTNode {
public:
    TypeSpec type;
    std::string name;
    bool isConst = false;          // elite modifier
    ExprPtr initializer;           // optional

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Assignment: target assignOp value
class AssignStmtNode : public ASTNode {
public:
    ExprPtr target;                // identifier, index, member, or deref
    TokenType op;                  // OP_ASSIGN, OP_PLUS_ASSIGN, etc.
    ExprPtr value;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// If statement: interrogate (cond) { then } [otherwise [interrogate (cond)] { else }]
class IfStmtNode : public ASTNode {
public:
    ExprPtr condition;
    std::unique_ptr<BlockNode> thenBlock;
    NodePtr elseBlock;             // either another IfStmtNode or a BlockNode

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Range for loop: impose (Type var from [start..end]) { body }
class ForRangeNode : public ASTNode {
public:
    TypeSpec varType;
    std::string varName;
    ExprPtr start;
    ExprPtr end;
    std::unique_ptr<BlockNode> body;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// For-each loop: impose (Type var from collection) { body }
class ForEachNode : public ASTNode {
public:
    TypeSpec varType;
    std::string varName;
    ExprPtr collection;
    std::unique_ptr<BlockNode> body;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// While loop: impose (condition) { body }
class WhileNode : public ASTNode {
public:
    ExprPtr condition;
    std::unique_ptr<BlockNode> body;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Infinite loop: impose forever { body }
class ForeverNode : public ASTNode {
public:
    std::unique_ptr<BlockNode> body;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Break: silence
class BreakNode : public ASTNode {
public:
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Continue: proceed
class ContinueNode : public ASTNode {
public:
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Return: report [expr]
class ReturnNode : public ASTNode {
public:
    ExprPtr value; // optional (void functions)

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Print: broadcast(expr)
class BroadcastNode : public ASTNode {
public:
    ExprPtr expression;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Input: demand(variable)
class DemandNode : public ASTNode {
public:
    ExprPtr target; // the variable to read into

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Memory free: kill expr
class KillNode : public ASTNode {
public:
    ExprPtr target;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Expression used as a statement (e.g. function calls).
class ExprStmtNode : public ASTNode {
public:
    ExprPtr expression;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// ============================================================================
// EXPRESSION NODES
// ============================================================================

// Binary expression: left op right
class BinaryExprNode : public ExprNode {
public:
    ExprPtr left;
    TokenType op;
    ExprPtr right;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Unary expression: op operand (prefix only: -, !, @, ^)
class UnaryExprNode : public ExprNode {
public:
    TokenType op;     // OP_MINUS, OP_NOT, OP_AT, OP_DEREF
    ExprPtr operand;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Integer literal
class IntLiteralNode : public ExprNode {
public:
    long long value;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Floating-point literal
class FloatLiteralNode : public ExprNode {
public:
    double value;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// String literal
class StringLiteralNode : public ExprNode {
public:
    std::string value;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Character literal
class CharLiteralNode : public ExprNode {
public:
    char value;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Boolean literal: loyal (true) / traitor (false)
class BoolLiteralNode : public ExprNode {
public:
    bool value;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Null literal: null
class NullLiteralNode : public ExprNode {
public:
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Identifier (variable reference)
class IdentifierNode : public ExprNode {
public:
    std::string name;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Function call: name(args)
class FuncCallNode : public ExprNode {
public:
    std::string name;
    std::vector<ExprPtr> arguments;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Method call: object.methodName(args)
class MethodCallNode : public ExprNode {
public:
    ExprPtr object;
    std::string methodName;
    std::vector<ExprPtr> arguments;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Member access: object.fieldName
class MemberAccessNode : public ExprNode {
public:
    ExprPtr object;
    std::string fieldName;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Array/map index access: object[index]
class IndexAccessNode : public ExprNode {
public:
    ExprPtr object;
    ExprPtr index;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Array literal: [expr1, expr2, ...]
class ArrayLiteralNode : public ExprNode {
public:
    std::vector<ExprPtr> elements;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Range literal: [start..end]  (used in array initialization)
class RangeLiteralNode : public ExprNode {
public:
    ExprPtr start;
    ExprPtr end;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Initializer list: {expr1, expr2, ...}  (used for struct initialization)
class InitListNode : public ExprNode {
public:
    std::vector<ExprPtr> values;

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

// Dynamic allocation: summon Type[(args)]
class SummonExprNode : public ExprNode {
public:
    TypeSpec type;
    std::vector<ExprPtr> arguments; // optional constructor args

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void print(int indent = 0) const override;
};

#endif // DICTATORSCRIPT_AST_NODE_H
