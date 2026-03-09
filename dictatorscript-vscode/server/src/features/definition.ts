/**
 * DictatorScript Go-to-Definition Handler
 *
 * Resolves the definition location for the identifier under the cursor.
 */

import {
    DefinitionParams,
    Location,
    Range,
    Position,
} from 'vscode-languageserver/node';
import { DocumentManager } from '../documentManager';
import { TokenType } from '../../../shared/src/tokenType';

/**
 * Handle a textDocument/definition request.
 *
 * Returns the Location of the symbol's declaration if the cursor is on an
 * Identifier token that resolves to a known symbol, or null otherwise.
 */
export function definitionHandler(
    params: DefinitionParams,
    docManager: DocumentManager
): Location | null {
    const uri = params.textDocument.uri;
    const line = params.position.line;
    const col = params.position.character;

    // Get the token at the cursor position
    const token = docManager.getTokenAtPosition(uri, line, col);
    if (!token || token.type !== TokenType.Identifier) {
        return null;
    }

    // Look up the symbol definition visible at this position
    const sym = docManager.findSymbolAtPosition(uri, line, col);
    if (!sym) {
        return null;
    }

    // Build the Location pointing to the symbol's declaration range
    return Location.create(
        uri,
        Range.create(
            Position.create(sym.line, sym.col),
            Position.create(sym.endLine, sym.endCol)
        )
    );
}
