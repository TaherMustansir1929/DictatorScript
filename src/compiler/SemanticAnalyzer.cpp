#include "SemanticAnalyzer.h"

SemanticAnalyzer::SemanticAnalyzer(const std::string& filename,
                                   ErrorReporter& errorReporter)
    : filename_(filename), errors_(errorReporter) {}

void SemanticAnalyzer::analyze(ProgramNode& program) {
    // First pass: register all struct types and function signatures
    // so they can be referenced before their definitions.
    registerStructs(program);
    registerFunctions(program);

    // Second pass: full analysis.
    program.accept(*this);
}

void SemanticAnalyzer::registerStructs(ProgramNode& program) {
    for (auto& decl : program.declarations) {
        if (auto* sd = dynamic_cast<StructDefNode*>(decl.get())) {
            SymbolInfo info;
            info.name = sd->name;
            info.type = TypeSpec::makeStruct(sd->name);
            info.isStruct = true;
            info.line = sd->line;
            info.col = sd->col;
            info.fields = sd->fields;
            if (!symbols_.declare(info)) {
                errors_.error(filename_, sd->line, sd->col,
                              "Duplicate struct definition: '" + sd->name + "'.");
            }
        }
    }
}

void SemanticAnalyzer::registerFunctions(ProgramNode& program) {
    for (auto& decl : program.declarations) {
        if (auto* fd = dynamic_cast<FunctionDefNode*>(decl.get())) {
            SymbolInfo info;
            info.name = fd->name;
            info.type = fd->returnType;
            info.isFunction = true;
            info.returnType = fd->returnType;
            info.line = fd->line;
            info.col = fd->col;
            for (const auto& param : fd->params) {
                info.paramTypes.push_back(param.type);
            }
            if (!symbols_.declare(info)) {
                errors_.error(filename_, fd->line, fd->col,
                              "Duplicate function definition: '" + fd->name + "'.");
            }
        }
    }
}

std::optional<TypeSpec> SemanticAnalyzer::resolveExprType(ExprNode& expr) {
    expr.accept(*this);
    return expr.resolvedType;
}

// ============================================================================
// Top-level visitors
// ============================================================================

void SemanticAnalyzer::visit(ProgramNode& node) {
    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }
}

void SemanticAnalyzer::visit(StructDefNode& node) {
    // Struct already registered in first pass. Validate fields.
    for (const auto& field : node.fields) {
        // Check that field types are valid.
        if (field.type.kind == TypeSpec::STRUCT) {
            auto sym = symbols_.lookup(field.type.name);
            if (!sym || !sym->isStruct) {
                errors_.error(filename_, field.line, field.col,
                              "Unknown type '" + field.type.name + "' for field '" + field.name + "'.");
            }
        }
    }
}

void SemanticAnalyzer::visit(FunctionDefNode& node) {
    // Function already registered in first pass.
    // Analyze the body with parameters in scope.
    auto savedReturn = currentReturnType_;
    currentReturnType_ = node.returnType;

    symbols_.enterScope();

    // Declare parameters.
    for (const auto& param : node.params) {
        SymbolInfo info;
        info.name = param.name;
        info.type = param.type;
        info.line = param.line;
        info.col = param.col;
        if (!symbols_.declare(info)) {
            errors_.error(filename_, param.line, param.col,
                          "Duplicate parameter name: '" + param.name + "'.");
        }
    }

    if (node.body) {
        // Visit statements directly (don't enter another scope from BlockNode).
        for (auto& stmt : node.body->statements) {
            stmt->accept(*this);
        }
    }

    symbols_.exitScope();
    currentReturnType_ = savedReturn;
}

void SemanticAnalyzer::visit(RegimeNode& node) {
    auto savedReturn = currentReturnType_;
    currentReturnType_ = TypeSpec::makePrimitive("int");

    symbols_.enterScope();
    if (node.body) {
        for (auto& stmt : node.body->statements) {
            stmt->accept(*this);
        }
    }
    symbols_.exitScope();

    currentReturnType_ = savedReturn;
}

void SemanticAnalyzer::visit(EnlistNode& /*node*/) {
    // Import handling is done by the Compiler. No semantic checks here.
}

// ============================================================================
// Statement visitors
// ============================================================================

void SemanticAnalyzer::visit(BlockNode& node) {
    symbols_.enterScope();
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    symbols_.exitScope();
}

void SemanticAnalyzer::visit(VarDeclNode& node) {
    // Check that the type is valid.
    if (node.type.kind == TypeSpec::STRUCT) {
        auto sym = symbols_.lookup(node.type.name);
        if (!sym || !sym->isStruct) {
            errors_.error(filename_, node.line, node.col,
                          "Unknown type '" + node.type.name + "'.");
        }
    }

    // Check initializer type if present.
    if (node.initializer) {
        resolveExprType(*node.initializer);
    }

    // Declare the variable.
    SymbolInfo info;
    info.name = node.name;
    info.type = node.type;
    info.isConst = node.isConst;
    info.line = node.line;
    info.col = node.col;

    if (!symbols_.declare(info)) {
        errors_.error(filename_, node.line, node.col,
                      "Duplicate variable declaration: '" + node.name + "'.");
    }
}

void SemanticAnalyzer::visit(AssignStmtNode& node) {
    if (node.target) {
        resolveExprType(*node.target);

        // Check for const mutation.
        if (auto* ident = dynamic_cast<IdentifierNode*>(node.target.get())) {
            auto sym = symbols_.lookup(ident->name);
            if (sym && sym->isConst) {
                errors_.error(filename_, node.line, node.col,
                              "Cannot assign to 'elite' (const) variable '" + ident->name + "'.");
            }
        }
    }

    if (node.value) {
        resolveExprType(*node.value);
    }
}

void SemanticAnalyzer::visit(IfStmtNode& node) {
    if (node.condition) {
        resolveExprType(*node.condition);
    }
    if (node.thenBlock) {
        node.thenBlock->accept(*this);
    }
    if (node.elseBlock) {
        node.elseBlock->accept(*this);
    }
}

void SemanticAnalyzer::visit(ForRangeNode& node) {
    if (node.start) resolveExprType(*node.start);
    if (node.end) resolveExprType(*node.end);

    loopDepth_++;
    symbols_.enterScope();

    // Declare the loop variable.
    SymbolInfo info;
    info.name = node.varName;
    info.type = node.varType;
    info.line = node.line;
    info.col = node.col;
    symbols_.declare(info);

    if (node.body) {
        for (auto& stmt : node.body->statements) {
            stmt->accept(*this);
        }
    }

    symbols_.exitScope();
    loopDepth_--;
}

void SemanticAnalyzer::visit(ForEachNode& node) {
    if (node.collection) resolveExprType(*node.collection);

    loopDepth_++;
    symbols_.enterScope();

    SymbolInfo info;
    info.name = node.varName;
    info.type = node.varType;
    info.line = node.line;
    info.col = node.col;
    symbols_.declare(info);

    if (node.body) {
        for (auto& stmt : node.body->statements) {
            stmt->accept(*this);
        }
    }

    symbols_.exitScope();
    loopDepth_--;
}

void SemanticAnalyzer::visit(WhileNode& node) {
    if (node.condition) resolveExprType(*node.condition);

    loopDepth_++;
    if (node.body) node.body->accept(*this);
    loopDepth_--;
}

void SemanticAnalyzer::visit(ForeverNode& node) {
    loopDepth_++;
    if (node.body) node.body->accept(*this);
    loopDepth_--;
}

void SemanticAnalyzer::visit(BreakNode& node) {
    if (loopDepth_ == 0) {
        errors_.error(filename_, node.line, node.col,
                      "'silence' (break) used outside of a loop.");
    }
}

void SemanticAnalyzer::visit(ContinueNode& node) {
    if (loopDepth_ == 0) {
        errors_.error(filename_, node.line, node.col,
                      "'proceed' (continue) used outside of a loop.");
    }
}

void SemanticAnalyzer::visit(ReturnNode& node) {
    if (node.value) {
        resolveExprType(*node.value);
    }

    if (currentReturnType_) {
        if (currentReturnType_->name == "void" && node.value) {
            errors_.error(filename_, node.line, node.col,
                          "Cannot return a value from a void function.");
        }
    }
}

void SemanticAnalyzer::visit(BroadcastNode& node) {
    if (node.expression) resolveExprType(*node.expression);
}

void SemanticAnalyzer::visit(DemandNode& node) {
    if (node.target) resolveExprType(*node.target);
}

void SemanticAnalyzer::visit(KillNode& node) {
    if (node.target) resolveExprType(*node.target);
}

void SemanticAnalyzer::visit(ExprStmtNode& node) {
    if (node.expression) resolveExprType(*node.expression);
}

// ============================================================================
// Expression visitors — resolve and annotate types
// ============================================================================

void SemanticAnalyzer::visit(BinaryExprNode& node) {
    if (node.left) resolveExprType(*node.left);
    if (node.right) resolveExprType(*node.right);

    // Infer result type from operands (simplified).
    if (node.left && node.left->resolvedType) {
        node.resolvedType = node.left->resolvedType;
    }

    // For comparison and logical operators, result is bool.
    if (node.op == TokenType::OP_EQ || node.op == TokenType::OP_NEQ ||
        node.op == TokenType::OP_LT || node.op == TokenType::OP_GT ||
        node.op == TokenType::OP_LTE || node.op == TokenType::OP_GTE ||
        node.op == TokenType::OP_AND || node.op == TokenType::OP_OR) {
        node.resolvedType = TypeSpec::makePrimitive("bool");
    }
}

void SemanticAnalyzer::visit(UnaryExprNode& node) {
    if (node.operand) resolveExprType(*node.operand);

    if (node.op == TokenType::OP_NOT) {
        node.resolvedType = TypeSpec::makePrimitive("bool");
    } else if (node.op == TokenType::OP_AT) {
        // Address-of: result is pointer to operand type.
        if (node.operand && node.operand->resolvedType) {
            node.resolvedType = TypeSpec::makePointer(*node.operand->resolvedType);
        }
    } else if (node.op == TokenType::OP_DEREF) {
        // Dereference: result is pointee type.
        if (node.operand && node.operand->resolvedType &&
            node.operand->resolvedType->kind == TypeSpec::POINTER &&
            node.operand->resolvedType->subType) {
            node.resolvedType = *node.operand->resolvedType->subType;
        }
    } else if (node.op == TokenType::OP_MINUS) {
        if (node.operand && node.operand->resolvedType) {
            node.resolvedType = node.operand->resolvedType;
        }
    }
}

void SemanticAnalyzer::visit(IntLiteralNode& node) {
    node.resolvedType = TypeSpec::makePrimitive("int");
}

void SemanticAnalyzer::visit(FloatLiteralNode& node) {
    node.resolvedType = TypeSpec::makePrimitive("float");
}

void SemanticAnalyzer::visit(StringLiteralNode& node) {
    node.resolvedType = TypeSpec::makePrimitive("string");
}

void SemanticAnalyzer::visit(CharLiteralNode& node) {
    node.resolvedType = TypeSpec::makePrimitive("char");
}

void SemanticAnalyzer::visit(BoolLiteralNode& node) {
    node.resolvedType = TypeSpec::makePrimitive("bool");
}

void SemanticAnalyzer::visit(NullLiteralNode& node) {
    node.resolvedType = TypeSpec::makePrimitive("void");
}

void SemanticAnalyzer::visit(IdentifierNode& node) {
    auto sym = symbols_.lookup(node.name);
    if (!sym) {
        errors_.error(filename_, node.line, node.col,
                      "Undeclared variable '" + node.name + "' used before declaration.");
    } else {
        node.resolvedType = sym->type;
    }
}

void SemanticAnalyzer::visit(FuncCallNode& node) {
    auto sym = symbols_.lookup(node.name);
    if (!sym) {
        errors_.error(filename_, node.line, node.col,
                      "Undeclared function '" + node.name + "'.");
    } else if (!sym->isFunction) {
        // Allow calling auto-typed variables (lambdas declared with `block`)
        // and struct-typed callables; only reject plain non-callable symbols.
        bool isCallable = (sym->type.kind == TypeSpec::AUTODEDUCE);
        if (!isCallable) {
            errors_.error(filename_, node.line, node.col,
                          "'" + node.name + "' is not a function.");
        }
        // Resolve type to void (we can't know the lambda's return type without
        // full type inference; the generated C++ is already correct).
        node.resolvedType = TypeSpec::makeVoid();
    } else {
        // Check argument count.
        if (node.arguments.size() != sym->paramTypes.size()) {
            errors_.error(filename_, node.line, node.col,
                          "Function '" + node.name + "' expects " +
                          std::to_string(sym->paramTypes.size()) + " arguments, got " +
                          std::to_string(node.arguments.size()) + ".");
        }
        node.resolvedType = sym->returnType;
    }

    // Visit arguments.
    for (auto& arg : node.arguments) {
        resolveExprType(*arg);
    }

}

void SemanticAnalyzer::visit(MethodCallNode& node) {
    if (node.object) resolveExprType(*node.object);
    for (auto& arg : node.arguments) {
        resolveExprType(*arg);
    }
    // Built-in method return types (simplified).
    if (node.methodName == "size") {
        node.resolvedType = TypeSpec::makePrimitive("int");
    } else if (node.methodName == "has" || node.methodName == "contains") {
        node.resolvedType = TypeSpec::makePrimitive("bool");
    } else if (node.methodName == "toUppercase" || node.methodName == "toLowercase") {
        if (node.object && node.object->resolvedType) {
            node.resolvedType = node.object->resolvedType;
        }
    }
}

void SemanticAnalyzer::visit(MemberAccessNode& node) {
    if (node.object) resolveExprType(*node.object);

    // Resolve field type from struct definition.
    if (node.object && node.object->resolvedType &&
        node.object->resolvedType->kind == TypeSpec::STRUCT) {
        auto sym = symbols_.lookup(node.object->resolvedType->name);
        if (sym && sym->isStruct) {
            bool found = false;
            for (const auto& field : sym->fields) {
                if (field.name == node.fieldName) {
                    node.resolvedType = field.type;
                    found = true;
                    break;
                }
            }
            if (!found) {
                errors_.error(filename_, node.line, node.col,
                              "Struct '" + node.object->resolvedType->name +
                              "' has no field '" + node.fieldName + "'.");
            }
        }
    }
}

void SemanticAnalyzer::visit(IndexAccessNode& node) {
    if (node.object) resolveExprType(*node.object);
    if (node.index) resolveExprType(*node.index);

    // Result type is the element type of the array/map.
    if (node.object && node.object->resolvedType) {
        if (node.object->resolvedType->kind == TypeSpec::ARRAY &&
            node.object->resolvedType->subType) {
            node.resolvedType = *node.object->resolvedType->subType;
        } else if (node.object->resolvedType->kind == TypeSpec::MAP &&
                   node.object->resolvedType->valType) {
            node.resolvedType = *node.object->resolvedType->valType;
        }
    }
}

void SemanticAnalyzer::visit(ArrayLiteralNode& node) {
    for (auto& elem : node.elements) {
        resolveExprType(*elem);
    }
    // Infer array type from first element.
    if (!node.elements.empty() && node.elements[0]->resolvedType) {
        node.resolvedType = TypeSpec::makeArray(*node.elements[0]->resolvedType);
    }
}

void SemanticAnalyzer::visit(RangeLiteralNode& node) {
    if (node.start) resolveExprType(*node.start);
    if (node.end) resolveExprType(*node.end);
    node.resolvedType = TypeSpec::makeArray(TypeSpec::makePrimitive("int"));
}

void SemanticAnalyzer::visit(InitListNode& node) {
    for (auto& val : node.values) {
        resolveExprType(*val);
    }
    // Type of init list is inferred from context (assignment/declaration).
}

void SemanticAnalyzer::visit(SummonExprNode& node) {
    for (auto& arg : node.arguments) {
        resolveExprType(*arg);
    }
    node.resolvedType = TypeSpec::makePointer(node.type);
}

// ============================================================================
// Feature node semantic visitors
// ============================================================================

// Feature 2: Lambda expression — block(params) -> retType { body }
void SemanticAnalyzer::visit(LambdaExprNode& node) {
    auto savedReturn = currentReturnType_;
    currentReturnType_ = node.returnType;

    symbols_.enterScope();
    for (const auto& param : node.params) {
        SymbolInfo info;
        info.name = param.name;
        info.type = param.type;
        info.line = param.line;
        info.col  = param.col;
        symbols_.declare(info);
    }
    if (node.body) {
        for (auto& stmt : node.body->statements) {
            stmt->accept(*this);
        }
    }
    symbols_.exitScope();
    currentReturnType_ = savedReturn;

    // Resolved type is a "callable" — we set it to the return type for simplicity.
    node.resolvedType = node.returnType;
}

// Feature 4: Spawn expression — spawn funcName(args)
void SemanticAnalyzer::visit(SpawnExprNode& node) {
    // Validate the function exists.
    auto sym = symbols_.lookup(node.funcName);
    if (!sym) {
        errors_.error(filename_, node.line, node.col,
                      "Undeclared function '" + node.funcName + "' passed to spawn.");
    }
    for (auto& arg : node.arguments) {
        resolveExprType(*arg);
    }
    // The thread type is represented as a pointer-to-void for type tracking purposes.
    node.resolvedType = TypeSpec::makeStruct("std::thread");
}

// Feature 5: Structured bindings — unpack [a, b, ...] = expr
void SemanticAnalyzer::visit(UnpackStmtNode& node) {
    if (node.expression) {
        resolveExprType(*node.expression);
    }
    // Declare each binding as an auto variable in current scope.
    for (const auto& name : node.bindings) {
        SymbolInfo info;
        info.name = name;
        info.type = TypeSpec(TypeSpec::AUTODEDUCE, "auto");
        info.line = node.line;
        info.col  = node.col;
        if (!symbols_.declare(info)) {
            errors_.error(filename_, node.line, node.col,
                          "Duplicate binding name '" + name + "' in unpack.");
        }
    }
}

