# DictatorScript Language Support

Welcome to the official Visual Studio Code extension for **DictatorScript**!

This extension provides comprehensive language support for the DictatorScript (`.ds`) programming language, helping you write "authoritative" code efficiently.

## Features

- **Syntax Highlighting**: Full semantic syntax highlighting for DictatorScript keywords, structures, and types.
- **Language Server (LSP)**: Integrated language server providing advanced features.
- **Linting & Diagnostics**: Real-time linting as you type, configurable via extension settings.
- **Hover Information**: Detailed hover documentation for symbols and types.
- **Go to Definition / References**: Seamlessly navigate your codebase.
- **Document Symbols**: Outline view support for quick file navigation.
- **Signature Help**: Parameter hints while writing function calls.
- **Snippets**: Useful code snippets for common DictatorScript constructs.

## Requirements

To use all features of this extension, you should have the DictatorScript compiler (`dsc`) available in your system `PATH`. Alternatively, you can configure the compiler path manually in your VS Code settings.

## Extension Settings

This extension contributes the following settings:

* `dictatorscript.compilerPath`: Set the path to the `dsc` compiler executable. If left empty, the extension will attempt to search your `PATH`.
* `dictatorscript.lint.enabled`: Enable/disable the built-in linter.
* `dictatorscript.lint.onType`: Run the linter as you type (with debounce).
* `dictatorscript.lint.onSave`: Run the linter automatically when a file is saved.
* `dictatorscript.lint.debounceMs`: Debounce delay (in milliseconds) for lint-on-type (default: 500ms).
* `dictatorscript.trace.server`: Enable trace logs between VS Code and the DictatorScript language server (options: `off`, `messages`, `verbose`).

## Known Issues

- Please report any issues or bugs to the [DictatorScript Repository](https://github.com/TaherMustansir1929/DictatorScript/issues).

## Release Notes

### 0.2.0
- Initial beta release with LSP support, diagnostics, syntax highlighting, and hover information.

---

**Enjoy writing code with absolute authority!**
