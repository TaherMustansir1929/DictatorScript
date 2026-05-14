---
title: VS Code Extension
description: Installing and using the DictatorScript VS Code Extension.
---

To provide a seamless developer experience, DictatorScript comes with a dedicated Visual Studio Code extension.

## Features

The extension currently supports:
- Full Syntax Highlighting for all DictatorScript keywords (`regime`, `declare`, `interrogate`, `summon`, etc.)
- Auto-completion snippets for common structures like `regime start` and `impose` loops.

## Installation

The DictatorScript extension will soon be available directly on the VS Code Extension Marketplace!

In the meantime, you can install the extension manually by compiling it from the source.

### Manual Installation

1. Ensure you have Node.js and npm installed.
2. Install the `vsce` packaging tool globally:
   ```bash
   npm install -g @vscode/vsce
   ```
3. Navigate to the extension folder within the DictatorScript project (e.g., `dictatorscript-vscode`).
4. Install dependencies and compile the extension:
   ```bash
   npm install
   npm run compile
   ```
5. Package the extension into a `.vsix` file:
   ```bash
   vsce package
   ```
6. Open VS Code, go to the Extensions view (`Ctrl+Shift+X` or `Cmd+Shift+X`).
7. Click the `...` menu at the top right of the Extensions view, select **Install from VSIX...**, and choose the generated `.vsix` file.

Once installed, simply open any `.ds` file to see the syntax highlighting in action!
