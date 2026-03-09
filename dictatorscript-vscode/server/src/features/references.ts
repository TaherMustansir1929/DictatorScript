/**
 * DictatorScript Find References Handler
 *
 * Locates all references to the symbol under the cursor,
 * including the definition site and all usage sites.
 */

import {
    ReferenceParams,
    Location,
    Range,
    Position,
} from 'vscode-languageserver/node';
import { DocumentManager } from '../documentManager';
import { TokenType } from '../../../shared/src/tokenType';

/**
 * Handle a textDocument/references request.
 */
export function referencesHandler(
    params: ReferenceParams,
    docManager: DocumentManager,
): Location[] {
    const uri = params.textDocument.uri;
    const line = params.position.line;
    const col = params.position.character;

    // Get the token at the cursor position
    const token = docManager.getTokenAtPosition(uri, line, col);
    if (!token || token.type !== TokenType.Identifier) {
        return [];
    }

    // Find the symbol definition for this identifier
    const symbol = docManager.findSymbolAtPosition(uri, line, col);
    if (!symbol) {
        return [];
    }

    const state = docManager.get(uri);
    if (!state) {
        return [];
    }

    const { parseResult } = state;

    // Determine if this symbol is a struct (fields can be accessed via member access)
    const isStructField = !symbol.isFunction && !symbol.isStruct && !symbol.isParameter
        && parseResult.symbols.some(s =>
            s.isStruct && s.fields.some(f => f.name === symbol.name)
        );

    const locations: Location[] = [];

    for (const ref of parseResult.references) {
        if (ref.name !== symbol.name) {
            continue;
        }

        // Skip member access references unless the symbol is a struct field
        if (ref.isMemberAccess && !isStructField) {
            continue;
        }

        const range = Range.create(
            Position.create(ref.line, ref.col),
            Position.create(ref.line, ref.col + ref.length),
        );

        locations.push(Location.create(uri, range));
    }

    return locations;
}
