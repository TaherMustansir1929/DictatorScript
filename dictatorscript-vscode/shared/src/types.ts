/**
 * DictatorScript Type System & Symbol Interfaces
 *
 * Mirrors the C++ TypeSpec, SymbolInfo, and SymbolTable structures
 * from src/compiler/ASTNode.h and src/compiler/SymbolTable.h.
 */

// ─── Type Specification ────────────────────────────────────────────────

export enum TypeKind {
    Primitive = 'PRIMITIVE',
    Array = 'ARRAY',
    Pointer = 'POINTER',
    Map = 'MAP',
    Struct = 'STRUCT',
}

export interface TypeSpec {
    kind: TypeKind;
    /** Base type name: "int", "float", "Student", etc. */
    name: string;
    /** Element type for ARRAY, pointee type for POINTER. */
    subType?: TypeSpec;
    /** Key type for MAP. */
    keyType?: TypeSpec;
    /** Value type for MAP. */
    valType?: TypeSpec;
}

export function makePrimitive(name: string): TypeSpec {
    return { kind: TypeKind.Primitive, name };
}

export function makeArray(elemType: TypeSpec): TypeSpec {
    return { kind: TypeKind.Array, name: 'array', subType: elemType };
}

export function makePointer(pointeeType: TypeSpec): TypeSpec {
    return { kind: TypeKind.Pointer, name: 'pointer', subType: pointeeType };
}

export function makeMap(keyType: TypeSpec, valType: TypeSpec): TypeSpec {
    return { kind: TypeKind.Map, name: 'map', keyType, valType };
}

export function makeStruct(name: string): TypeSpec {
    return { kind: TypeKind.Struct, name };
}

export function typeToString(t: TypeSpec): string {
    switch (t.kind) {
        case TypeKind.Primitive:
            return t.name;
        case TypeKind.Array:
            return `${t.subType ? typeToString(t.subType) : '?'}[]`;
        case TypeKind.Pointer:
            return `${t.subType ? typeToString(t.subType) : '?'}->`;
        case TypeKind.Map:
            return `map<${t.keyType ? typeToString(t.keyType) : '?'}, ${t.valType ? typeToString(t.valType) : '?'}>`;
        case TypeKind.Struct:
            return t.name;
    }
}

// ─── Struct Fields ─────────────────────────────────────────────────────

export interface StructField {
    type: TypeSpec;
    name: string;
    line: number;
    col: number;
}

// ─── Function Parameters ───────────────────────────────────────────────

export interface Parameter {
    type: TypeSpec;
    name: string;
    line: number;
    col: number;
}

// ─── Symbol Info ───────────────────────────────────────────────────────

export interface SymbolInfo {
    name: string;
    type: TypeSpec;
    isConst: boolean;
    isFunction: boolean;
    isStruct: boolean;
    isParameter: boolean;

    /** Declaration position (0-based). */
    line: number;
    col: number;
    /** End of the declaration name (for highlighting). */
    endLine: number;
    endCol: number;

    /** For functions: parameter list. */
    params: Parameter[];
    /** For functions: return type. */
    returnType?: TypeSpec;
    /** For structs: field list. */
    fields: StructField[];

    /** Scope depth where this symbol was declared (0 = global). */
    scopeDepth: number;
}

// ─── Symbol Reference ──────────────────────────────────────────────────

export interface SymbolReference {
    /** Name of the symbol being referenced. */
    name: string;
    /** 0-based line. */
    line: number;
    /** 0-based column. */
    col: number;
    /** Length of the identifier. */
    length: number;
    /** True if this is the declaration site. */
    isDefinition: boolean;
    /** True if preceded by a dot (member/method access). */
    isMemberAccess: boolean;
}

// ─── Scope Info ────────────────────────────────────────────────────────

export interface ScopeInfo {
    startLine: number;
    startCol: number;
    endLine: number;
    endCol: number;
    /** Index into parent scope array, -1 for global. */
    parentIndex: number;
    /** Symbols declared directly in this scope. */
    symbols: SymbolInfo[];
}

// ─── Lexer Diagnostic ──────────────────────────────────────────────────

export interface LexerDiagnostic {
    line: number;
    col: number;
    endCol: number;
    message: string;
    severity: 'error' | 'warning';
}

// ─── Parser Diagnostic ─────────────────────────────────────────────────

export interface ParserDiagnostic {
    line: number;
    col: number;
    endLine: number;
    endCol: number;
    message: string;
    severity: 'error' | 'warning' | 'info';
    code?: string;
    /** For code actions — the suggested replacement text. */
    data?: { replacement?: string };
}

// ─── Parse Result ──────────────────────────────────────────────────────

export interface ParseResult {
    symbols: SymbolInfo[];
    references: SymbolReference[];
    scopes: ScopeInfo[];
    diagnostics: ParserDiagnostic[];
    importPaths: string[];
}
