/**
 * DictatorScript Compiler Integration
 *
 * Runs the actual `dsc` compiler in diagnostic mode and parses its error output
 * to provide real compiler-grade diagnostics alongside the built-in linter.
 */

import * as vscode from 'vscode';
import * as cp from 'child_process';
import * as path from 'path';

/**
 * Attempt to run the dsc compiler on a file and parse diagnostics from its output.
 * Returns an array of diagnostics, or null if the compiler is not available.
 */
export function runCompilerDiagnostics(
    filePath: string,
    diagnosticCollection: vscode.DiagnosticCollection
): void {
    const config = vscode.workspace.getConfiguration('dictatorscript');
    const compilerPath = config.get<string>('compilerPath', '');

    const dsc = compilerPath || 'dsc';

    // Run with --dump-ast which triggers lexing + parsing + semantic analysis
    // but doesn't actually compile to C++
    const args = [filePath, '--dump-ast'];

    try {
        const proc = cp.spawn(dsc, args, {
            cwd: path.dirname(filePath),
            timeout: 10000, // 10 second timeout
        });

        let stderr = '';
        let stdout = '';

        proc.stderr.on('data', (data: Buffer) => {
            stderr += data.toString();
        });

        proc.stdout.on('data', (data: Buffer) => {
            stdout += data.toString();
        });

        proc.on('close', (code) => {
            if (code !== 0 && stderr) {
                const diags = parseCompilerOutput(stderr, filePath);
                if (diags.length > 0) {
                    const uri = vscode.Uri.file(filePath);
                    // Merge with existing diagnostics
                    const existing = diagnosticCollection.get(uri) || [];
                    const nonCompilerDiags = [...existing].filter(d => d.source !== 'dsc');
                    diagnosticCollection.set(uri, [...nonCompilerDiags, ...diags]);
                }
            }
        });

        proc.on('error', () => {
            // Compiler not found — silently ignore
        });
    } catch {
        // Compiler not available
    }
}

/**
 * Parse error output from the dsc compiler.
 * Expected format: [DictatorScript Error] filename.ds:LINE:COL -- message
 */
function parseCompilerOutput(output: string, filePath: string): vscode.Diagnostic[] {
    const diagnostics: vscode.Diagnostic[] = [];
    const errorPattern = /\[DictatorScript (Error|Warning)\]\s+([^:]+):(\d+):(\d+)\s*--\s*(.+)/g;

    let match;
    while ((match = errorPattern.exec(output)) !== null) {
        const severity = match[1] === 'Error'
            ? vscode.DiagnosticSeverity.Error
            : vscode.DiagnosticSeverity.Warning;
        const line = Math.max(0, parseInt(match[3], 10) - 1); // Convert 1-based to 0-based
        const col = Math.max(0, parseInt(match[4], 10) - 1);
        const message = match[5].trim();

        const range = new vscode.Range(line, col, line, col + 1);
        const diag = new vscode.Diagnostic(range, message, severity);
        diag.source = 'dsc';
        diagnostics.push(diag);
    }

    return diagnostics;
}
