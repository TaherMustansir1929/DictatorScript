/**
 * DictatorScript Compiler Integration
 *
 * Runs the actual `dsc` compiler in diagnostic mode and parses its error output.
 * No VS Code dependencies — uses only Node.js APIs and LSP types.
 */

import * as cp from 'child_process';
import * as path from 'path';
import { Diagnostic, DiagnosticSeverity, Range, Position } from 'vscode-languageserver/node';

/**
 * Run the dsc compiler on a file and return diagnostics parsed from its output.
 */
export function runCompilerDiagnostics(
    filePath: string,
    compilerPath: string
): Promise<Diagnostic[]> {
    return new Promise((resolve) => {
        const dsc = compilerPath || 'dsc';
        const args = [filePath, '--dump-ast'];

        try {
            const proc = cp.spawn(dsc, args, {
                cwd: path.dirname(filePath),
                timeout: 10000,
            });

            let stderr = '';

            proc.stderr.on('data', (data: Buffer) => {
                stderr += data.toString();
            });

            proc.on('close', (code) => {
                if (code !== 0 && stderr) {
                    resolve(parseCompilerOutput(stderr));
                } else {
                    resolve([]);
                }
            });

            proc.on('error', () => {
                resolve([]); // Compiler not found — silently ignore
            });
        } catch {
            resolve([]);
        }
    });
}

/**
 * Parse error output from the dsc compiler.
 * Format: [DictatorScript Error] filename.ds:LINE:COL -- message
 */
function parseCompilerOutput(output: string): Diagnostic[] {
    const diagnostics: Diagnostic[] = [];
    const errorPattern = /\[DictatorScript (Error|Warning)\]\s+([^:]+):(\d+):(\d+)\s*--\s*(.+)/g;

    let match;
    while ((match = errorPattern.exec(output)) !== null) {
        const severity = match[1] === 'Error'
            ? DiagnosticSeverity.Error
            : DiagnosticSeverity.Warning;
        const line = Math.max(0, parseInt(match[3], 10) - 1); // 1-based to 0-based
        const col = Math.max(0, parseInt(match[4], 10) - 1);
        const message = match[5].trim();

        diagnostics.push({
            range: Range.create(Position.create(line, col), Position.create(line, col + 1)),
            message,
            severity,
            source: 'dsc',
        });
    }

    return diagnostics;
}
