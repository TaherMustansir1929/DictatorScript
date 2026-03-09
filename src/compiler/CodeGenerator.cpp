#include "CodeGenerator.h"
#include <algorithm>

CodeGenerator::CodeGenerator(ErrorReporter& errorReporter)
    : errors_(errorReporter) {}

// ============================================================================
// Helpers
// ============================================================================

void CodeGenerator::emit(const std::string& text) {
    output_ << text;
}

void CodeGenerator::emitLine(const std::string& text) {
    emitIndent();
    output_ << text << "\n";
}

void CodeGenerator::emitIndent() {
    for (int i = 0; i < indentLevel_; i++) {
        output_ << "    ";
    }
}

void CodeGenerator::indent() { indentLevel_++; }
void CodeGenerator::dedent() { if (indentLevel_ > 0) indentLevel_--; }

std::string CodeGenerator::popExpr() {
    if (exprStack_.empty()) return "/* error: missing expression */";
    std::string result = std::move(exprStack_.back());
    exprStack_.pop_back();
    return result;
}

std::string CodeGenerator::toCppType(const TypeSpec& type) const {
    switch (type.kind) {
        case TypeSpec::PRIMITIVE:
            if (type.name == "string") return "std::string";
            return type.name; // int, float, double, char, bool, void
        case TypeSpec::ARRAY:
            if (type.subType) return "std::vector<" + toCppType(*type.subType) + ">";
            return "std::vector<int>";
        case TypeSpec::POINTER:
            if (type.subType) return toCppType(*type.subType) + "*";
            return "void*";
        case TypeSpec::MAP:
            if (type.keyType && type.valType)
                return "std::unordered_map<" + toCppType(*type.keyType) + ", " +
                       toCppType(*type.valType) + ">";
            return "std::unordered_map<int, int>";
        case TypeSpec::STRUCT:
            return type.name;
    }
    return "void";
}

std::string CodeGenerator::getZeroValue(const TypeSpec& type) const {
    if (type.kind == TypeSpec::POINTER) return "nullptr";
    if (type.kind == TypeSpec::ARRAY || type.kind == TypeSpec::MAP) return ""; // default-constructed
    if (type.name == "int" || type.name == "double" || type.name == "float") return "0";
    if (type.name == "char") return "'\\0'";
    if (type.name == "bool") return "false";
    if (type.name == "string") return "\"\"";
    if (type.kind == TypeSpec::STRUCT) return "{}";
    return "0";
}

std::string CodeGenerator::escapeString(const std::string& s) const {
    std::string result;
    for (char c : s) {
        switch (c) {
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '\\': result += "\\\\"; break;
            case '"':  result += "\\\""; break;
            case '\0': result += "\\0"; break;
            default:   result += c; break;
        }
    }
    return result;
}

std::string CodeGenerator::escapeChar(char c) const {
    switch (c) {
        case '\n': return "\\n";
        case '\t': return "\\t";
        case '\\': return "\\\\";
        case '\'': return "\\'";
        case '\0': return "\\0";
        default: return std::string(1, c);
    }
}

std::string CodeGenerator::operatorToString(TokenType op) const {
    switch (op) {
        case TokenType::OP_PLUS:         return "+";
        case TokenType::OP_MINUS:        return "-";
        case TokenType::OP_STAR:         return "*";
        case TokenType::OP_SLASH:        return "/";
        case TokenType::OP_PERCENT:      return "%";
        case TokenType::OP_EQ:           return "==";
        case TokenType::OP_NEQ:          return "!=";
        case TokenType::OP_LT:           return "<";
        case TokenType::OP_GT:           return ">";
        case TokenType::OP_LTE:          return "<=";
        case TokenType::OP_GTE:          return ">=";
        case TokenType::OP_AND:          return "&&";
        case TokenType::OP_OR:           return "||";
        case TokenType::OP_NOT:          return "!";
        case TokenType::OP_ASSIGN:       return "=";
        case TokenType::OP_PLUS_ASSIGN:  return "+=";
        case TokenType::OP_MINUS_ASSIGN: return "-=";
        case TokenType::OP_STAR_ASSIGN:  return "*=";
        case TokenType::OP_SLASH_ASSIGN: return "/=";
        default: return "?";
    }
}

void CodeGenerator::flattenPlusChain(ExprNode* expr, std::vector<ExprNode*>& parts) {
    if (auto* bin = dynamic_cast<BinaryExprNode*>(expr)) {
        if (bin->op == TokenType::OP_PLUS) {
            flattenPlusChain(bin->left.get(), parts);
            flattenPlusChain(bin->right.get(), parts);
            return;
        }
    }
    parts.push_back(expr);
}

void CodeGenerator::emitHeaders() {
    emitLine("#include <iostream>");
    emitLine("#include <string>");
    emitLine("#include <vector>");
    emitLine("#include <unordered_map>");
    emitLine("#include <algorithm>");
    emitLine("#include <cctype>");
    emit("\n");
}

void CodeGenerator::emitForwardDeclarations(ProgramNode& program) {
    // Forward declare all structs.
    for (auto& decl : program.declarations) {
        if (auto* sd = dynamic_cast<StructDefNode*>(decl.get())) {
            emitLine("struct " + sd->name + ";");
        }
    }
    // Forward declare all functions.
    for (auto& decl : program.declarations) {
        if (auto* fd = dynamic_cast<FunctionDefNode*>(decl.get())) {
            emitIndent();
            emit(toCppType(fd->returnType) + " " + fd->name + "(");
            for (size_t i = 0; i < fd->params.size(); i++) {
                if (i > 0) emit(", ");
                emit(toCppType(fd->params[i].type) + " " + fd->params[i].name);
            }
            emit(");\n");
        }
    }
    emit("\n");
}

// ============================================================================
// Main entry point
// ============================================================================

std::string CodeGenerator::generate(ProgramNode& program) {
    output_.str("");
    output_.clear();
    indentLevel_ = 0;
    exprStack_.clear();

    emitHeaders();
    emitForwardDeclarations(program);

    // Emit struct definitions first, then functions, then regime.
    for (auto& decl : program.declarations) {
        if (dynamic_cast<StructDefNode*>(decl.get())) {
            decl->accept(*this);
            emit("\n");
        }
    }
    for (auto& decl : program.declarations) {
        if (dynamic_cast<FunctionDefNode*>(decl.get())) {
            decl->accept(*this);
            emit("\n");
        }
    }
    for (auto& decl : program.declarations) {
        if (dynamic_cast<RegimeNode*>(decl.get())) {
            decl->accept(*this);
            emit("\n");
        }
    }

    return output_.str();
}

// ============================================================================
// Top-level visitors
// ============================================================================

void CodeGenerator::visit(ProgramNode& /*node*/) {
    // Handled by generate() directly.
}

void CodeGenerator::visit(StructDefNode& node) {
    emitLine("struct " + node.name + " {");
    indent();
    for (const auto& field : node.fields) {
        emitLine(toCppType(field.type) + " " + field.name + ";");
    }
    dedent();
    emitLine("};");
}

void CodeGenerator::visit(FunctionDefNode& node) {
    emitIndent();
    emit(toCppType(node.returnType) + " " + node.name + "(");
    for (size_t i = 0; i < node.params.size(); i++) {
        if (i > 0) emit(", ");
        emit(toCppType(node.params[i].type) + " " + node.params[i].name);
    }
    emit(") {\n");
    indent();
    if (node.body) {
        for (auto& stmt : node.body->statements) {
            stmt->accept(*this);
        }
    }
    dedent();
    emitLine("}");
}

void CodeGenerator::visit(RegimeNode& node) {
    emitLine("int main() {");
    indent();
    if (node.body) {
        for (auto& stmt : node.body->statements) {
            stmt->accept(*this);
        }
    }
    emit("\n");
    emitLine("return 0;");
    dedent();
    emitLine("}");
}

void CodeGenerator::visit(EnlistNode& /*node*/) {
    // Imports are resolved by the Compiler before code generation.
    // Imported declarations are already merged into the AST.
}

// ============================================================================
// Statement visitors
// ============================================================================

void CodeGenerator::visit(BlockNode& node) {
    emitLine("{");
    indent();
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    dedent();
    emitLine("}");
}

void CodeGenerator::visit(VarDeclNode& node) {
    emitIndent();
    if (node.isConst) emit("const ");
    emit(toCppType(node.type) + " " + node.name);

    if (node.initializer) {
        // Check if initializer is a range literal — needs special handling.
        if (auto* range = dynamic_cast<RangeLiteralNode*>(node.initializer.get())) {
            emit(";\n");
            // Generate loop to populate the vector.
            range->start->accept(*this);
            std::string startStr = popExpr();
            range->end->accept(*this);
            std::string endStr = popExpr();
            emitLine("for (int _i = " + startStr + "; _i <= " + endStr +
                     "; _i++) " + node.name + ".push_back(_i);");
            return;
        }
        emit(" = ");
        node.initializer->accept(*this);
        emit(popExpr());
    } else {
        // Auto-zero initialization.
        std::string zero = getZeroValue(node.type);
        if (!zero.empty()) {
            emit(" = " + zero);
        }
    }
    emit(";\n");
}

void CodeGenerator::visit(AssignStmtNode& node) {
    emitIndent();
    node.target->accept(*this);
    emit(popExpr());
    emit(" " + operatorToString(node.op) + " ");
    node.value->accept(*this);
    emit(popExpr());
    emit(";\n");
}

void CodeGenerator::visit(IfStmtNode& node) {
    // Iterative approach to handle arbitrary-length if/else-if/else chains.
    IfStmtNode* currentIf = &node;
    bool isFirst = true;

    while (currentIf) {
        emitIndent();
        if (isFirst) {
            emit("if (");
        } else {
            emit("} else if (");
        }
        currentIf->condition->accept(*this);
        emit(popExpr());
        emit(") {\n");
        indent();
        if (currentIf->thenBlock) {
            for (auto& stmt : currentIf->thenBlock->statements) {
                stmt->accept(*this);
            }
        }
        dedent();
        isFirst = false;

        if (currentIf->elseBlock) {
            if (auto* nextIf = dynamic_cast<IfStmtNode*>(currentIf->elseBlock.get())) {
                currentIf = nextIf;
                continue;
            } else {
                // Plain else block.
                emitIndent();
                emit("} else {\n");
                indent();
                if (auto* block = dynamic_cast<BlockNode*>(currentIf->elseBlock.get())) {
                    for (auto& stmt : block->statements) {
                        stmt->accept(*this);
                    }
                }
                dedent();
                emitLine("}");
                return;
            }
        } else {
            emitLine("}");
            return;
        }
    }
}

void CodeGenerator::visit(ForRangeNode& node) {
    node.start->accept(*this);
    std::string startStr = popExpr();
    node.end->accept(*this);
    std::string endStr = popExpr();

    emitLine("for (" + toCppType(node.varType) + " " + node.varName +
             " = " + startStr + "; " + node.varName + " <= " + endStr +
             "; " + node.varName + "++) {");
    indent();
    if (node.body) {
        for (auto& stmt : node.body->statements) {
            stmt->accept(*this);
        }
    }
    dedent();
    emitLine("}");
}

void CodeGenerator::visit(ForEachNode& node) {
    node.collection->accept(*this);
    std::string collStr = popExpr();

    emitLine("for (" + toCppType(node.varType) + " " + node.varName +
             " : " + collStr + ") {");
    indent();
    if (node.body) {
        for (auto& stmt : node.body->statements) {
            stmt->accept(*this);
        }
    }
    dedent();
    emitLine("}");
}

void CodeGenerator::visit(WhileNode& node) {
    emitIndent();
    emit("while (");
    node.condition->accept(*this);
    emit(popExpr());
    emit(") {\n");
    indent();
    if (node.body) {
        for (auto& stmt : node.body->statements) {
            stmt->accept(*this);
        }
    }
    dedent();
    emitLine("}");
}

void CodeGenerator::visit(ForeverNode& node) {
    emitLine("while (true) {");
    indent();
    if (node.body) {
        for (auto& stmt : node.body->statements) {
            stmt->accept(*this);
        }
    }
    dedent();
    emitLine("}");
}

void CodeGenerator::visit(BreakNode& /*node*/) {
    emitLine("break;");
}

void CodeGenerator::visit(ContinueNode& /*node*/) {
    emitLine("continue;");
}

void CodeGenerator::visit(ReturnNode& node) {
    emitIndent();
    if (node.value) {
        emit("return ");
        node.value->accept(*this);
        emit(popExpr());
        emit(";\n");
    } else {
        emit("return;\n");
    }
}

void CodeGenerator::visit(BroadcastNode& node) {
    emitIndent();
    emit("std::cout");

    // Flatten + chains for clean << output.
    std::vector<ExprNode*> parts;
    flattenPlusChain(node.expression.get(), parts);

    for (auto* part : parts) {
        emit(" << ");
        part->accept(*this);
        emit(popExpr());
    }
    emit(";\n");
}

void CodeGenerator::visit(DemandNode& node) {
    emitIndent();
    node.target->accept(*this);
    std::string targetStr = popExpr();

    // Check if the target is a string type — use getline.
    if (node.target->resolvedType &&
        node.target->resolvedType->name == "string" &&
        node.target->resolvedType->kind == TypeSpec::PRIMITIVE) {
        emit("std::getline(std::cin, " + targetStr + ");\n");
    } else {
        emit("std::cin >> " + targetStr + ";\n");
    }
}

void CodeGenerator::visit(KillNode& node) {
    emitIndent();
    emit("delete ");
    node.target->accept(*this);
    emit(popExpr());
    emit(";\n");
}

void CodeGenerator::visit(ExprStmtNode& node) {
    emitIndent();
    node.expression->accept(*this);
    emit(popExpr());
    emit(";\n");
}

// ============================================================================
// Expression visitors — push results onto exprStack_
// ============================================================================

void CodeGenerator::visit(BinaryExprNode& node) {
    node.left->accept(*this);
    std::string left = popExpr();
    node.right->accept(*this);
    std::string right = popExpr();
    exprStack_.push_back(left + " " + operatorToString(node.op) + " " + right);
}

void CodeGenerator::visit(UnaryExprNode& node) {
    node.operand->accept(*this);
    std::string operand = popExpr();

    if (node.op == TokenType::OP_AT) {
        exprStack_.push_back("&" + operand);
    } else if (node.op == TokenType::OP_DEREF) {
        exprStack_.push_back("*" + operand);
    } else if (node.op == TokenType::OP_NOT) {
        exprStack_.push_back("!" + operand);
    } else if (node.op == TokenType::OP_MINUS) {
        exprStack_.push_back("-" + operand);
    } else {
        exprStack_.push_back(operatorToString(node.op) + operand);
    }
}

void CodeGenerator::visit(IntLiteralNode& node) {
    exprStack_.push_back(std::to_string(node.value));
}

void CodeGenerator::visit(FloatLiteralNode& node) {
    exprStack_.push_back(std::to_string(node.value));
}

void CodeGenerator::visit(StringLiteralNode& node) {
    exprStack_.push_back("\"" + escapeString(node.value) + "\"");
}

void CodeGenerator::visit(CharLiteralNode& node) {
    exprStack_.push_back("'" + escapeChar(node.value) + "'");
}

void CodeGenerator::visit(BoolLiteralNode& node) {
    exprStack_.push_back(node.value ? "true" : "false");
}

void CodeGenerator::visit(NullLiteralNode& /*node*/) {
    exprStack_.push_back("nullptr");
}

void CodeGenerator::visit(IdentifierNode& node) {
    exprStack_.push_back(node.name);
}

void CodeGenerator::visit(FuncCallNode& node) {
    std::vector<std::string> args;
    for (auto& arg : node.arguments) {
        arg->accept(*this);
        args.push_back(popExpr());
    }

    std::string result = node.name + "(";
    for (size_t i = 0; i < args.size(); i++) {
        if (i > 0) result += ", ";
        result += args[i];
    }
    result += ")";
    exprStack_.push_back(result);
}

void CodeGenerator::visit(MethodCallNode& node) {
    node.object->accept(*this);
    std::string obj = popExpr();

    // Collect arguments.
    std::vector<std::string> args;
    for (auto& arg : node.arguments) {
        arg->accept(*this);
        args.push_back(popExpr());
    }

    // Handle built-in method translations.
    if (node.methodName == "add") {
        // arr.add(x) → arr.push_back(x)
        std::string result = obj + ".push_back(";
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0) result += ", ";
            result += args[i];
        }
        result += ")";
        exprStack_.push_back(result);
    } else if (node.methodName == "remove") {
        // For arrays: arr.remove(i) → arr.erase(arr.begin() + i)
        // For maps: map.remove(key) → map.erase(key)
        if (node.object->resolvedType &&
            node.object->resolvedType->kind == TypeSpec::MAP) {
            exprStack_.push_back(obj + ".erase(" + (args.empty() ? "" : args[0]) + ")");
        } else {
            exprStack_.push_back(obj + ".erase(" + obj + ".begin() + " +
                                 (args.empty() ? "0" : args[0]) + ")");
        }
    } else if (node.methodName == "size") {
        exprStack_.push_back(obj + ".size()");
    } else if (node.methodName == "has") {
        // map.has(key) → map.count(key) > 0
        exprStack_.push_back(obj + ".count(" + (args.empty() ? "" : args[0]) + ") > 0");
    } else if (node.methodName == "contains") {
        // str.contains(sub) → str.find(sub) != std::string::npos
        exprStack_.push_back(obj + ".find(" + (args.empty() ? "" : args[0]) +
                             ") != std::string::npos");
    } else if (node.methodName == "toUppercase") {
        // Check if it's a char or string.
        if (node.object->resolvedType &&
            node.object->resolvedType->name == "char") {
            exprStack_.push_back("(char)std::toupper(" + obj + ")");
        } else {
            // String: generate inline transformation.
            exprStack_.push_back("[](std::string s) { std::transform(s.begin(), s.end(), s.begin(), ::toupper); return s; }(" + obj + ")");
        }
    } else if (node.methodName == "toLowercase") {
        if (node.object->resolvedType &&
            node.object->resolvedType->name == "char") {
            exprStack_.push_back("(char)std::tolower(" + obj + ")");
        } else {
            exprStack_.push_back("[](std::string s) { std::transform(s.begin(), s.end(), s.begin(), ::tolower); return s; }(" + obj + ")");
        }
    } else {
        // Unknown method — emit as-is.
        std::string result = obj + "." + node.methodName + "(";
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0) result += ", ";
            result += args[i];
        }
        result += ")";
        exprStack_.push_back(result);
    }
}

void CodeGenerator::visit(MemberAccessNode& node) {
    node.object->accept(*this);
    std::string obj = popExpr();
    exprStack_.push_back(obj + "." + node.fieldName);
}

void CodeGenerator::visit(IndexAccessNode& node) {
    node.object->accept(*this);
    std::string obj = popExpr();
    node.index->accept(*this);
    std::string idx = popExpr();
    exprStack_.push_back(obj + "[" + idx + "]");
}

void CodeGenerator::visit(ArrayLiteralNode& node) {
    std::string result = "{";
    for (size_t i = 0; i < node.elements.size(); i++) {
        node.elements[i]->accept(*this);
        if (i > 0) result += ", ";
        result += popExpr();
    }
    result += "}";
    exprStack_.push_back(result);
}

void CodeGenerator::visit(RangeLiteralNode& node) {
    // Range literals as expressions produce the vector inline.
    // Usually handled specially in VarDeclNode, but if used in other contexts:
    node.start->accept(*this);
    std::string startStr = popExpr();
    node.end->accept(*this);
    std::string endStr = popExpr();
    exprStack_.push_back("/* range " + startStr + ".." + endStr + " */");
}

void CodeGenerator::visit(InitListNode& node) {
    std::string result = "{";
    for (size_t i = 0; i < node.values.size(); i++) {
        node.values[i]->accept(*this);
        if (i > 0) result += ", ";
        result += popExpr();
    }
    result += "}";
    exprStack_.push_back(result);
}

void CodeGenerator::visit(SummonExprNode& node) {
    std::string result = "new " + toCppType(node.type);
    if (!node.arguments.empty()) {
        result += "(";
        for (size_t i = 0; i < node.arguments.size(); i++) {
            node.arguments[i]->accept(*this);
            if (i > 0) result += ", ";
            result += popExpr();
        }
        result += ")";
    }
    exprStack_.push_back(result);
}
