/**
 * DictatorScript Rename Handler
 *
 * Supports prepare-rename validation and rename execution,
 * replacing all references to a symbol with a new name.
 */

import {
    PrepareRenameParams,
    RenameParams,
    WorkspaceEdit,
    TextEdit,
    Range,
    Position,
} from 'vscode-languageserver/node';
import { DocumentManager } from '../documentManager';
import { TokenType } from '../../../shared/src/tokenType';
import { ALL_KEYWORDS } from '../../../shared/src/languageData';

/** Valid DictatorScript identifier pattern. */
const IDENTIFIER_REGEX = /^[a-zA-Z_][a-zA-Z0-9_]*$/;

/**
 * Handle a textDocument/prepareRename request.
 *
 * Returns the range of the token if it is renameable, or null if not.
 */
export function prepareRenameHandler(
    params: PrepareRenameParams,
    docManager: DocumentManager,
): Range | null {
    const uri = params.textDocument.uri;
    const line = params.position.line;
    const col = params.position.character;

    const token = docManager.getTokenAtPosition(uri, line, col);
    if (!token || token.type !== TokenType.Identifier) {
        return null;
    }

    // Cannot rename keywords
    if (ALL_KEYWORDS.has(token.lexeme)) {
        return null;
    }

    return Range.create(
        Position.create(token.line, token.col),
        Position.create(token.line, token.col + token.length),
    );
}

/**
 * Handle a textDocument/rename request.
 *
 * Validates the new name, finds all references, and builds a WorkspaceEdit.
 */
export function renameHandler(
    params: RenameParams,
    docManager: DocumentManager,
): WorkspaceEdit | null {
    const uri = params.textDocument.uri;
    const line = params.position.line;
    const col = params.position.character;
    const newName = params.newName;

    // Validate new name is a valid identifier
    if (!IDENTIFIER_REGEX.test(newName)) {
        return null;
    }

    // Validate new name is not a keyword
    if (ALL_KEYWORDS.has(newName)) {
        return null;
    }

    // Find the symbol at the cursor position
    const symbol = docManager.findSymbolAtPosition(uri, line, col);
    if (!symbol) {
        return null;
    }

    const state = docManager.get(uri);
    if (!state) {
        return null;
    }

    const { parseResult } = state;
    const edits: TextEdit[] = [];
    const seen = new Set<string>(); // Deduplicate edits by "line:col"

    // Walk all references matching the symbol name
    for (const ref of parseResult.references) {
        if (ref.name !== symbol.name) {
            continue;
        }

        // Only include non-member-access references (direct references to this symbol)
        if (ref.isMemberAccess) {
            continue;
        }

        const key = `${ref.line}:${ref.col}`;
        if (seen.has(key)) {
            continue;
        }
        seen.add(key);

        const range = Range.create(
            Position.create(ref.line, ref.col),
            Position.create(ref.line, ref.col + ref.length),
        );

        edits.push(TextEdit.replace(range, newName));
    }

    // Ensure the definition site is included (it should already be in references,
    // but add it if not present as a safety measure)
    const defKey = `${symbol.line}:${symbol.col}`;
    if (!seen.has(defKey)) {
        const defRange = Range.create(
            Position.create(symbol.line, symbol.col),
            Position.create(symbol.endLine, symbol.endCol),
        );
        edits.push(TextEdit.replace(defRange, newName));
    }

    if (edits.length === 0) {
        return null;
    }

    const workspaceEdit: WorkspaceEdit = {
        changes: {
            [uri]: edits,
        },
    };

    return workspaceEdit;
}
