/**
 * DictatorScript Completion Provider
 *
 * Provides context-aware autocompletion for keywords, built-in methods,
 * user-defined symbols, and struct fields.
 */

import {
    CompletionParams,
    CompletionItem,
    CompletionItemKind,
} from 'vscode-languageserver/node';
import { DocumentManager } from '../documentManager';
import { DS_KEYWORDS, BUILTIN_METHODS } from '../../../shared/src/languageData';
import { typeToString, TypeKind } from '../../../shared/src/types';

// ─── Main Handler ───────────────────────────────────────────────────────

/**
 * Compute completion items for the given cursor position.
 */
export function completionHandler(
    params: CompletionParams,
    docManager: DocumentManager,
): CompletionItem[] {
    const state = docManager.get(params.textDocument.uri);
    if (!state) return [];

    const line = params.position.line;
    const col = params.position.character;

    // Get the text of the current line up to the cursor
    const fullText = state.document.getText();
    const lines = fullText.split('\n');
    if (line >= lines.length) return [];
    const lineText = lines[line].substring(0, col);

    // ── Dot-completion (member access / built-in methods) ──────────

    if (lineText.trimEnd().endsWith('.')) {
        return dotCompletions(lineText, line, col, params.textDocument.uri, docManager);
    }

    // ── Import path completion (placeholder) ───────────────────────

    if (/import\s+"[^"]*$/.test(lineText)) {
        // Placeholder: return empty for now
        return [];
    }

    // ── General completions: keywords + user symbols ───────────────

    return [
        ...keywordCompletions(),
        ...symbolCompletions(params.textDocument.uri, line, col, docManager),
    ];
}

// ─── Dot Completion ─────────────────────────────────────────────────────

function dotCompletions(
    lineText: string,
    line: number,
    col: number,
    uri: string,
    docManager: DocumentManager,
): CompletionItem[] {
    const items: CompletionItem[] = [];

    // Always offer built-in methods
    for (const [name, info] of Object.entries(BUILTIN_METHODS)) {
        items.push({
            label: name,
            kind: CompletionItemKind.Method,
            detail: info.signature,
            documentation: `${info.description}\n\nApplies to: ${info.appliesTo}`,
        });
    }

    // Try to resolve the identifier before the dot to check for struct fields
    const beforeDot = lineText.trimEnd().slice(0, -1); // remove trailing dot
    const identMatch = beforeDot.match(/(\w+)\s*$/);
    if (identMatch) {
        const identName = identMatch[1];
        // Find the token position: approximate the column of the identifier
        const identCol = beforeDot.lastIndexOf(identName);
        const symbol = docManager.findSymbolAtPosition(uri, line, identCol);

        if (symbol && symbol.type.kind === TypeKind.Struct) {
            // Look up the struct definition to get fields
            const structName = symbol.type.name;
            const state = docManager.get(uri);
            if (state) {
                const structSymbol = state.parseResult.symbols.find(
                    s => s.isStruct && s.name === structName
                );
                if (structSymbol) {
                    for (const field of structSymbol.fields) {
                        items.push({
                            label: field.name,
                            kind: CompletionItemKind.Field,
                            detail: typeToString(field.type),
                            documentation: `Field of struct '${structName}'`,
                        });
                    }
                }
            }
        }
    }

    return items;
}

// ─── Keyword Completions ────────────────────────────────────────────────

function keywordCompletions(): CompletionItem[] {
    const items: CompletionItem[] = [];

    // Type keywords get CompletionItemKind.TypeParameter
    for (const kw of DS_KEYWORDS.types) {
        items.push({
            label: kw,
            kind: CompletionItemKind.TypeParameter,
            detail: 'Type keyword',
        });
    }

    // All other keyword categories get CompletionItemKind.Keyword
    const otherCategories = [
        DS_KEYWORDS.declarations,
        DS_KEYWORDS.entryPoint,
        DS_KEYWORDS.booleans,
        DS_KEYWORDS.controlFlow,
        DS_KEYWORDS.io,
        DS_KEYWORDS.memory,
        DS_KEYWORDS.imports,
    ] as const;

    for (const category of otherCategories) {
        for (const kw of category) {
            items.push({
                label: kw,
                kind: CompletionItemKind.Keyword,
                detail: 'Keyword',
            });
        }
    }

    return items;
}

// ─── Symbol Completions ─────────────────────────────────────────────────

function symbolCompletions(
    uri: string,
    line: number,
    col: number,
    docManager: DocumentManager,
): CompletionItem[] {
    const items: CompletionItem[] = [];
    const visibleSymbols = docManager.getVisibleSymbolsAt(uri, line, col);

    for (const sym of visibleSymbols) {
        let kind: CompletionItemKind;
        let detail: string;

        if (sym.isFunction) {
            kind = CompletionItemKind.Function;
            const paramList = sym.params
                .map(p => `${typeToString(p.type)} ${p.name}`)
                .join(', ');
            const retStr = sym.returnType ? typeToString(sym.returnType) : 'void';
            detail = `command ${sym.name}(${paramList}) -> ${retStr}`;
        } else if (sym.isStruct) {
            kind = CompletionItemKind.Struct;
            detail = `law ${sym.name}`;
        } else if (sym.isConst) {
            kind = CompletionItemKind.Constant;
            detail = `declare elite ${typeToString(sym.type)} ${sym.name}`;
        } else {
            kind = CompletionItemKind.Variable;
            detail = `${typeToString(sym.type)} ${sym.name}`;
        }

        items.push({
            label: sym.name,
            kind,
            detail,
        });
    }

    return items;
}
