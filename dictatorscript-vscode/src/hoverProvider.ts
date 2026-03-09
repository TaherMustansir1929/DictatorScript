/**
 * DictatorScript Hover Provider
 *
 * Shows documentation when hovering over keywords, types, and built-in methods.
 */

import * as vscode from 'vscode';
import { KEYWORD_DOCS, BUILTIN_METHODS } from './languageData';

export class DSHoverProvider implements vscode.HoverProvider {
    provideHover(
        document: vscode.TextDocument,
        position: vscode.Position,
        _token: vscode.CancellationToken
    ): vscode.ProviderResult<vscode.Hover> {
        const wordRange = document.getWordRangeAtPosition(position, /[a-zA-Z_][a-zA-Z0-9_]*/);
        if (!wordRange) return null;

        const word = document.getText(wordRange);

        // Check keyword documentation
        if (KEYWORD_DOCS[word]) {
            return new vscode.Hover(
                new vscode.MarkdownString(KEYWORD_DOCS[word]),
                wordRange
            );
        }

        // Check built-in methods (when preceded by a dot)
        if (BUILTIN_METHODS[word]) {
            const charBefore = wordRange.start.character > 0
                ? document.getText(new vscode.Range(
                    position.line, wordRange.start.character - 1,
                    position.line, wordRange.start.character
                ))
                : '';
            if (charBefore === '.') {
                const method = BUILTIN_METHODS[word];
                const md = new vscode.MarkdownString();
                md.appendCodeblock(method.signature, 'dictatorscript');
                md.appendMarkdown(`\n\n${method.description}\n\n*Applies to: ${method.appliesTo}*`);
                return new vscode.Hover(md, wordRange);
            }
        }

        return null;
    }
}
