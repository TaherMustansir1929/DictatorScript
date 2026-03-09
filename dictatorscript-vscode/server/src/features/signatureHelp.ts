/**
 * DictatorScript Signature Help Handler
 *
 * Provides function signature information and highlights the active parameter
 * as the user types arguments inside a function call.
 */

import {
    SignatureHelpParams,
    SignatureHelp,
    SignatureInformation,
    ParameterInformation,
} from 'vscode-languageserver/node';
import { DocumentManager } from '../documentManager';
import { TokenType } from '../../../shared/src/tokenType';
import { typeToString } from '../../../shared/src/types';

/**
 * Handle a textDocument/signatureHelp request.
 *
 * Scans backward from the cursor to find the enclosing function call,
 * resolves it against user-defined symbols and built-ins, counts commas
 * to determine the active parameter, and returns the signature information.
 */
export function signatureHelpHandler(
    params: SignatureHelpParams,
    docManager: DocumentManager
): SignatureHelp | null {
    const uri = params.textDocument.uri;
    const line = params.position.line;
    const col = params.position.character;

    const state = docManager.get(uri);
    if (!state) return null;

    const { tokens, parseResult } = state;

    // Find the index of the last token that starts at or before the cursor
    let cursorIdx = -1;
    for (let i = 0; i < tokens.length; i++) {
        const t = tokens[i];
        if (t.type === TokenType.EOF) break;
        if (t.line < line || (t.line === line && t.col <= col)) {
            cursorIdx = i;
        } else {
            break;
        }
    }

    if (cursorIdx < 0) return null;

    // Scan backward from the cursor to find the nearest unmatched '('
    let depth = 0;
    let openParenIdx = -1;
    let commaCount = 0;

    for (let i = cursorIdx; i >= 0; i--) {
        const t = tokens[i];

        if (t.type === TokenType.RParen) {
            depth++;
        } else if (t.type === TokenType.LParen) {
            if (depth === 0) {
                openParenIdx = i;
                break;
            }
            depth--;
        } else if (t.type === TokenType.Comma && depth === 0) {
            commaCount++;
        }
    }

    if (openParenIdx < 0) return null;

    // Find the function name token immediately before the '('
    // Skip any newline tokens between the name and the paren
    let nameIdx = openParenIdx - 1;
    while (nameIdx >= 0 && tokens[nameIdx].type === TokenType.Newline) {
        nameIdx--;
    }

    if (nameIdx < 0) return null;

    const nameToken = tokens[nameIdx];
    let signatureLabel: string | null = null;
    let paramInfos: ParameterInformation[] = [];

    // Check if it's a user-defined function
    if (nameToken.type === TokenType.Identifier) {
        const funcSym = parseResult.symbols.find(
            s => s.isFunction && s.name === nameToken.lexeme
        );

        if (funcSym) {
            // Build parameter labels
            const paramLabels = funcSym.params.map(
                p => `${typeToString(p.type)} ${p.name}`
            );

            paramInfos = paramLabels.map(
                label => ParameterInformation.create(label)
            );

            const returnSuffix = funcSym.returnType
                ? ` -> ${typeToString(funcSym.returnType)}`
                : '';

            signatureLabel = `${funcSym.name}(${paramLabels.join(', ')})${returnSuffix}`;
        }
    }

    // Check built-in: broadcast(expression)
    if (!signatureLabel && nameToken.type === TokenType.KW_Broadcast) {
        signatureLabel = 'broadcast(expression)';
        paramInfos = [ParameterInformation.create('expression')];
    }

    // Check built-in: demand(variable)
    if (!signatureLabel && nameToken.type === TokenType.KW_Demand) {
        signatureLabel = 'demand(variable)';
        paramInfos = [ParameterInformation.create('variable')];
    }

    if (!signatureLabel) return null;

    const signatureInfo = SignatureInformation.create(
        signatureLabel,
        undefined,
        ...paramInfos
    );

    return {
        signatures: [signatureInfo],
        activeSignature: 0,
        activeParameter: commaCount,
    };
}
