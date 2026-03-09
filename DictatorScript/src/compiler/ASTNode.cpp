#include "ASTNode.h"
#include <iostream>

// ============================================================================
// TypeSpec implementation
// ============================================================================

std::string TypeSpec::toString() const {
    switch (kind) {
        case PRIMITIVE:
            return name;
        case ARRAY:
            return subType ? subType->toString() + "[]" : name + "[]";
        case POINTER:
            return subType ? subType->toString() + "->" : name + "->";
        case MAP:
            return "map<" + (keyType ? keyType->toString() : "?") + ", " +
                   (valType ? valType->toString() : "?") + ">";
        case STRUCT:
            return name;
    }
    return "unknown";
}

bool TypeSpec::operator==(const TypeSpec& other) const {
    if (kind != other.kind) return false;
    if (name != other.name) return false;
    if ((subType == nullptr) != (other.subType == nullptr)) return false;
    if (subType && *subType != *other.subType) return false;
    if ((keyType == nullptr) != (other.keyType == nullptr)) return false;
    if (keyType && *keyType != *other.keyType) return false;
    if ((valType == nullptr) != (other.valType == nullptr)) return false;
    if (valType && *valType != *other.valType) return false;
    return true;
}

TypeSpec TypeSpec::makePrimitive(const std::string& name) {
    return TypeSpec(PRIMITIVE, name);
}

TypeSpec TypeSpec::makeArray(const TypeSpec& elemType) {
    TypeSpec t(ARRAY, elemType.name);
    t.subType = std::make_shared<TypeSpec>(elemType);
    return t;
}

TypeSpec TypeSpec::makePointer(const TypeSpec& pointeeType) {
    TypeSpec t(POINTER, pointeeType.name);
    t.subType = std::make_shared<TypeSpec>(pointeeType);
    return t;
}

TypeSpec TypeSpec::makeMap(const TypeSpec& key, const TypeSpec& val) {
    TypeSpec t(MAP, "map");
    t.keyType = std::make_shared<TypeSpec>(key);
    t.valType = std::make_shared<TypeSpec>(val);
    return t;
}

TypeSpec TypeSpec::makeStruct(const std::string& name) {
    return TypeSpec(STRUCT, name);
}

TypeSpec TypeSpec::makeVoid() {
    return TypeSpec(PRIMITIVE, "void");
}

// ============================================================================
// ASTNode helpers
// ============================================================================

void ASTNode::printIndent(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
}

// ============================================================================
// Top-level node print implementations
// ============================================================================

void ProgramNode::print(int indent) const {
    printIndent(indent);
    std::cout << "Program" << std::endl;
    for (const auto& decl : declarations) {
        decl->print(indent + 1);
    }
}

void StructDefNode::print(int indent) const {
    printIndent(indent);
    std::cout << "StructDef: " << name << std::endl;
    for (const auto& field : fields) {
        printIndent(indent + 1);
        std::cout << "Field: " << field.type.toString() << " " << field.name << std::endl;
    }
}

void FunctionDefNode::print(int indent) const {
    printIndent(indent);
    std::cout << "FunctionDef: " << name << " -> " << returnType.toString() << std::endl;
    for (const auto& param : params) {
        printIndent(indent + 1);
        std::cout << "Param: " << param.type.toString() << " " << param.name << std::endl;
    }
    if (body) body->print(indent + 1);
}

void RegimeNode::print(int indent) const {
    printIndent(indent);
    std::cout << "Regime (main)" << std::endl;
    if (body) body->print(indent + 1);
}

void EnlistNode::print(int indent) const {
    printIndent(indent);
    std::cout << "Enlist: \"" << filename << "\"" << std::endl;
}

// ============================================================================
// Statement node print implementations
// ============================================================================

void BlockNode::print(int indent) const {
    printIndent(indent);
    std::cout << "Block" << std::endl;
    for (const auto& stmt : statements) {
        stmt->print(indent + 1);
    }
}

void VarDeclNode::print(int indent) const {
    printIndent(indent);
    std::cout << "VarDecl: " << (isConst ? "elite " : "")
              << type.toString() << " " << name << std::endl;
    if (initializer) {
        printIndent(indent + 1);
        std::cout << "Init:" << std::endl;
        initializer->print(indent + 2);
    }
}

void AssignStmtNode::print(int indent) const {
    printIndent(indent);
    std::cout << "Assign: " << tokenTypeToString(op) << std::endl;
    if (target) {
        printIndent(indent + 1);
        std::cout << "Target:" << std::endl;
        target->print(indent + 2);
    }
    if (value) {
        printIndent(indent + 1);
        std::cout << "Value:" << std::endl;
        value->print(indent + 2);
    }
}

void IfStmtNode::print(int indent) const {
    printIndent(indent);
    std::cout << "If" << std::endl;
    if (condition) {
        printIndent(indent + 1);
        std::cout << "Condition:" << std::endl;
        condition->print(indent + 2);
    }
    if (thenBlock) {
        printIndent(indent + 1);
        std::cout << "Then:" << std::endl;
        thenBlock->print(indent + 2);
    }
    if (elseBlock) {
        printIndent(indent + 1);
        std::cout << "Else:" << std::endl;
        elseBlock->print(indent + 2);
    }
}

void ForRangeNode::print(int indent) const {
    printIndent(indent);
    std::cout << "ForRange: " << varType.toString() << " " << varName << std::endl;
    if (start) { printIndent(indent + 1); std::cout << "Start:" << std::endl; start->print(indent + 2); }
    if (end)   { printIndent(indent + 1); std::cout << "End:" << std::endl;   end->print(indent + 2);   }
    if (body)  body->print(indent + 1);
}

void ForEachNode::print(int indent) const {
    printIndent(indent);
    std::cout << "ForEach: " << varType.toString() << " " << varName << std::endl;
    if (collection) { printIndent(indent + 1); std::cout << "Collection:" << std::endl; collection->print(indent + 2); }
    if (body) body->print(indent + 1);
}

void WhileNode::print(int indent) const {
    printIndent(indent);
    std::cout << "While" << std::endl;
    if (condition) { printIndent(indent + 1); std::cout << "Condition:" << std::endl; condition->print(indent + 2); }
    if (body) body->print(indent + 1);
}

void ForeverNode::print(int indent) const {
    printIndent(indent);
    std::cout << "Forever" << std::endl;
    if (body) body->print(indent + 1);
}

void BreakNode::print(int indent) const {
    printIndent(indent);
    std::cout << "Break" << std::endl;
}

void ContinueNode::print(int indent) const {
    printIndent(indent);
    std::cout << "Continue" << std::endl;
}

void ReturnNode::print(int indent) const {
    printIndent(indent);
    std::cout << "Return" << std::endl;
    if (value) value->print(indent + 1);
}

void BroadcastNode::print(int indent) const {
    printIndent(indent);
    std::cout << "Broadcast" << std::endl;
    if (expression) expression->print(indent + 1);
}

void DemandNode::print(int indent) const {
    printIndent(indent);
    std::cout << "Demand" << std::endl;
    if (target) target->print(indent + 1);
}

void KillNode::print(int indent) const {
    printIndent(indent);
    std::cout << "Kill" << std::endl;
    if (target) target->print(indent + 1);
}

void ExprStmtNode::print(int indent) const {
    printIndent(indent);
    std::cout << "ExprStmt" << std::endl;
    if (expression) expression->print(indent + 1);
}

// ============================================================================
// Expression node print implementations
// ============================================================================

void BinaryExprNode::print(int indent) const {
    printIndent(indent);
    std::cout << "BinaryExpr: " << tokenTypeToString(op) << std::endl;
    if (left)  left->print(indent + 1);
    if (right) right->print(indent + 1);
}

void UnaryExprNode::print(int indent) const {
    printIndent(indent);
    std::cout << "UnaryExpr: " << tokenTypeToString(op) << std::endl;
    if (operand) operand->print(indent + 1);
}

void IntLiteralNode::print(int indent) const {
    printIndent(indent);
    std::cout << "IntLiteral: " << value << std::endl;
}

void FloatLiteralNode::print(int indent) const {
    printIndent(indent);
    std::cout << "FloatLiteral: " << value << std::endl;
}

void StringLiteralNode::print(int indent) const {
    printIndent(indent);
    std::cout << "StringLiteral: \"" << value << "\"" << std::endl;
}

void CharLiteralNode::print(int indent) const {
    printIndent(indent);
    std::cout << "CharLiteral: '" << value << "'" << std::endl;
}

void BoolLiteralNode::print(int indent) const {
    printIndent(indent);
    std::cout << "BoolLiteral: " << (value ? "loyal" : "traitor") << std::endl;
}

void NullLiteralNode::print(int indent) const {
    printIndent(indent);
    std::cout << "NullLiteral" << std::endl;
}

void IdentifierNode::print(int indent) const {
    printIndent(indent);
    std::cout << "Identifier: " << name << std::endl;
}

void FuncCallNode::print(int indent) const {
    printIndent(indent);
    std::cout << "FuncCall: " << name << std::endl;
    for (const auto& arg : arguments) {
        arg->print(indent + 1);
    }
}

void MethodCallNode::print(int indent) const {
    printIndent(indent);
    std::cout << "MethodCall: ." << methodName << std::endl;
    if (object) { printIndent(indent + 1); std::cout << "Object:" << std::endl; object->print(indent + 2); }
    for (const auto& arg : arguments) {
        arg->print(indent + 1);
    }
}

void MemberAccessNode::print(int indent) const {
    printIndent(indent);
    std::cout << "MemberAccess: ." << fieldName << std::endl;
    if (object) object->print(indent + 1);
}

void IndexAccessNode::print(int indent) const {
    printIndent(indent);
    std::cout << "IndexAccess" << std::endl;
    if (object) { printIndent(indent + 1); std::cout << "Object:" << std::endl; object->print(indent + 2); }
    if (index)  { printIndent(indent + 1); std::cout << "Index:" << std::endl;  index->print(indent + 2);  }
}

void ArrayLiteralNode::print(int indent) const {
    printIndent(indent);
    std::cout << "ArrayLiteral" << std::endl;
    for (const auto& elem : elements) {
        elem->print(indent + 1);
    }
}

void RangeLiteralNode::print(int indent) const {
    printIndent(indent);
    std::cout << "RangeLiteral" << std::endl;
    if (start) { printIndent(indent + 1); std::cout << "Start:" << std::endl; start->print(indent + 2); }
    if (end)   { printIndent(indent + 1); std::cout << "End:" << std::endl;   end->print(indent + 2);   }
}

void InitListNode::print(int indent) const {
    printIndent(indent);
    std::cout << "InitList" << std::endl;
    for (const auto& val : values) {
        val->print(indent + 1);
    }
}

void SummonExprNode::print(int indent) const {
    printIndent(indent);
    std::cout << "Summon: " << type.toString() << std::endl;
    for (const auto& arg : arguments) {
        arg->print(indent + 1);
    }
}
