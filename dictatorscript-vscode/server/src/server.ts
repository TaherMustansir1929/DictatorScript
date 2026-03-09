/**
 * DictatorScript Language Server — Entry Point
 *
 * Creates the LSP connection, registers all capabilities and feature handlers,
 * and manages document lifecycle events.
 */

import {
    createConnection,
    TextDocuments,
    ProposedFeatures,
    InitializeParams,
    InitializeResult,
    TextDocumentSyncKind,
    DiagnosticSeverity,
    SemanticTokensRegistrationType,
} from 'vscode-languageserver/node';
import { TextDocument } from 'vscode-languageserver-textdocument';

import { DocumentManager } from './documentManager';
import { runCompilerDiagnostics } from './compilerIntegration';

// Feature handlers
import { computeDiagnostics } from './features/diagnostics';
import { completionHandler } from './features/completion';
import { hoverHandler } from './features/hover';
import { definitionHandler } from './features/definition';
import { documentSymbolHandler } from './features/documentSymbol';
import { signatureHelpHandler } from './features/signatureHelp';
import { tokenTypes, tokenModifiers, semanticTokensHandler } from './features/semanticTokens';
import { referencesHandler } from './features/references';
import { prepareRenameHandler, renameHandler } from './features/rename';
import { codeActionHandler } from './features/codeAction';

// ─── Setup ──────────────────────────────────────────────────────────

const connection = createConnection(ProposedFeatures.all);
const documents = new TextDocuments(TextDocument);
const docManager = new DocumentManager();

let compilerPath = '';

// ─── Initialize ─────────────────────────────────────────────────────

connection.onInitialize((_params: InitializeParams): InitializeResult => {
    return {
        capabilities: {
            textDocumentSync: TextDocumentSyncKind.Full,
            completionProvider: {
                triggerCharacters: ['.', '"', '<'],
                resolveProvider: false,
            },
            hoverProvider: true,
            definitionProvider: true,
            referencesProvider: true,
            documentSymbolProvider: true,
            renameProvider: {
                prepareProvider: true,
            },
            signatureHelpProvider: {
                triggerCharacters: ['(', ','],
                retriggerCharacters: [','],
            },
            codeActionProvider: {
                codeActionKinds: ['quickfix'],
            },
            semanticTokensProvider: {
                legend: {
                    tokenTypes,
                    tokenModifiers,
                },
                full: true,
            },
        },
    };
});

connection.onInitialized(() => {
    // Pull initial configuration
    connection.workspace.getConfiguration('dictatorscript').then((config) => {
        if (config) {
            compilerPath = config.compilerPath || '';
        }
    });
});

// ─── Document Lifecycle ─────────────────────────────────────────────

documents.onDidChangeContent((change) => {
    const state = docManager.update(change.document);
    const diagnostics = computeDiagnostics(state);
    connection.sendDiagnostics({
        uri: change.document.uri,
        diagnostics,
    });
});

documents.onDidSave((event) => {
    // Run compiler diagnostics on save for deeper analysis
    const uri = event.document.uri;
    if (uri.startsWith('file://')) {
        let filePath = uri.replace('file:///', '').replace(/\//g, '\\');
        // Handle percent-encoded spaces, etc.
        filePath = decodeURIComponent(filePath);

        runCompilerDiagnostics(filePath, compilerPath).then((compilerDiags) => {
            if (compilerDiags.length > 0) {
                const state = docManager.get(uri);
                if (state) {
                    const lintDiags = computeDiagnostics(state);
                    connection.sendDiagnostics({
                        uri,
                        diagnostics: [...lintDiags, ...compilerDiags],
                    });
                }
            }
        });
    }
});

documents.onDidClose((event) => {
    docManager.remove(event.document.uri);
    connection.sendDiagnostics({ uri: event.document.uri, diagnostics: [] });
});

// ─── Configuration ──────────────────────────────────────────────────

connection.onDidChangeConfiguration((_change) => {
    connection.workspace.getConfiguration('dictatorscript').then((config) => {
        if (config) {
            compilerPath = config.compilerPath || '';
        }
    });

    // Re-validate all open documents
    documents.all().forEach((doc) => {
        const state = docManager.update(doc);
        const diagnostics = computeDiagnostics(state);
        connection.sendDiagnostics({ uri: doc.uri, diagnostics });
    });
});

// ─── Feature Handlers ───────────────────────────────────────────────

connection.onCompletion((params) => completionHandler(params, docManager));

connection.onHover((params) => hoverHandler(params, docManager));

connection.onDefinition((params) => definitionHandler(params, docManager));

connection.onReferences((params) => referencesHandler(params, docManager));

connection.onDocumentSymbol((params) => documentSymbolHandler(params, docManager));

connection.onSignatureHelp((params) => signatureHelpHandler(params, docManager));

connection.onCodeAction((params) => codeActionHandler(params, docManager));

connection.onPrepareRename((params) => prepareRenameHandler(params, docManager));

connection.onRenameRequest((params) => renameHandler(params, docManager));

connection.languages.semanticTokens.on((params) => semanticTokensHandler(params, docManager));

// ─── Start ──────────────────────────────────────────────────────────

documents.listen(connection);
connection.listen();
