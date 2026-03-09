/**
 * DictatorScript Code Action Provider
 *
 * Provides quick-fix code actions for diagnostics, such as
 * replacing C++ keywords with their DictatorScript equivalents.
 */

import {
    CodeActionParams,
    CodeAction,
    CodeActionKind,
    TextEdit,
} from 'vscode-languageserver/node';
import { DocumentManager } from '../documentManager';

/**
 * Handle a textDocument/codeAction request.
 *
 * Walks the diagnostics in the request context and produces quick-fix
 * CodeAction entries for diagnostics that have a known replacement.
 */
export function codeActionHandler(
    params: CodeActionParams,
    docManager: DocumentManager,
): CodeAction[] {
    const actions: CodeAction[] = [];

    for (const diagnostic of params.context.diagnostics) {
        // Only handle diagnostics with the 'cpp-keyword' code
        if (diagnostic.code !== 'cpp-keyword') {
            continue;
        }

        // The diagnostic data should contain the replacement suggestion
        const data = diagnostic.data as { replacement?: string } | undefined;
        if (!data || !data.replacement) {
            continue;
        }

        const replacement = data.replacement;

        const action: CodeAction = {
            title: `Replace with '${replacement}'`,
            kind: CodeActionKind.QuickFix,
            diagnostics: [diagnostic],
            isPreferred: true,
            edit: {
                changes: {
                    [params.textDocument.uri]: [
                        TextEdit.replace(diagnostic.range, replacement),
                    ],
                },
            },
        };

        actions.push(action);
    }

    return actions;
}
