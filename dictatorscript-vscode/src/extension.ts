/**
 * DictatorScript VSCode Extension — Entry Point
 *
 * Wires together:
 * - Built-in linter (lint-on-type and lint-on-save)
 * - Compiler integration (optional, if dsc is available)
 * - Hover provider (keyword + method docs)
 */

import * as vscode from 'vscode';
import { lintDocument } from './linter';
import { DSHoverProvider } from './hoverProvider';
import { runCompilerDiagnostics } from './compilerIntegration';

const LANGUAGE_ID = 'dictatorscript';

let diagnosticCollection: vscode.DiagnosticCollection;
let compilerDiagnosticCollection: vscode.DiagnosticCollection;
let debounceTimer: ReturnType<typeof setTimeout> | undefined;

export function activate(context: vscode.ExtensionContext) {
    diagnosticCollection = vscode.languages.createDiagnosticCollection('dictatorscript-lint');
    compilerDiagnosticCollection = vscode.languages.createDiagnosticCollection('dictatorscript-compiler');
    context.subscriptions.push(diagnosticCollection, compilerDiagnosticCollection);

    // ── Hover Provider ──────────────────────────────────────────────────
    context.subscriptions.push(
        vscode.languages.registerHoverProvider(LANGUAGE_ID, new DSHoverProvider())
    );

    // ── Lint on open ────────────────────────────────────────────────────
    if (vscode.window.activeTextEditor?.document.languageId === LANGUAGE_ID) {
        runLint(vscode.window.activeTextEditor.document);
    }

    // ── Lint on document open ───────────────────────────────────────────
    context.subscriptions.push(
        vscode.workspace.onDidOpenTextDocument(doc => {
            if (doc.languageId === LANGUAGE_ID) {
                runLint(doc);
            }
        })
    );

    // ── Lint on save ────────────────────────────────────────────────────
    context.subscriptions.push(
        vscode.workspace.onDidSaveTextDocument(doc => {
            if (doc.languageId !== LANGUAGE_ID) return;
            const config = vscode.workspace.getConfiguration('dictatorscript');
            if (!config.get<boolean>('lint.enabled', true)) return;
            if (!config.get<boolean>('lint.onSave', true)) return;

            runLint(doc);
            // Also run compiler diagnostics on save (heavier operation)
            if (doc.uri.scheme === 'file') {
                runCompilerDiagnostics(doc.uri.fsPath, compilerDiagnosticCollection);
            }
        })
    );

    // ── Lint on type (debounced) ────────────────────────────────────────
    context.subscriptions.push(
        vscode.workspace.onDidChangeTextDocument(event => {
            if (event.document.languageId !== LANGUAGE_ID) return;
            const config = vscode.workspace.getConfiguration('dictatorscript');
            if (!config.get<boolean>('lint.enabled', true)) return;
            if (!config.get<boolean>('lint.onType', true)) return;

            const delay = config.get<number>('lint.debounceMs', 500);
            if (debounceTimer) clearTimeout(debounceTimer);
            debounceTimer = setTimeout(() => {
                runLint(event.document);
            }, delay);
        })
    );

    // ── Clean up on close ───────────────────────────────────────────────
    context.subscriptions.push(
        vscode.workspace.onDidCloseTextDocument(doc => {
            diagnosticCollection.delete(doc.uri);
            compilerDiagnosticCollection.delete(doc.uri);
        })
    );

    // ── Re-lint all open DS files when config changes ───────────────────
    context.subscriptions.push(
        vscode.workspace.onDidChangeConfiguration(event => {
            if (event.affectsConfiguration('dictatorscript')) {
                for (const editor of vscode.window.visibleTextEditors) {
                    if (editor.document.languageId === LANGUAGE_ID) {
                        runLint(editor.document);
                    }
                }
            }
        })
    );
}

function runLint(doc: vscode.TextDocument) {
    const config = vscode.workspace.getConfiguration('dictatorscript');
    if (!config.get<boolean>('lint.enabled', true)) {
        diagnosticCollection.delete(doc.uri);
        return;
    }

    const diagnostics = lintDocument(doc);
    diagnosticCollection.set(doc.uri, diagnostics);
}

export function deactivate() {
    if (debounceTimer) {
        clearTimeout(debounceTimer);
    }
}
