/**
 * DictatorScript Language Data
 *
 * Central registry of all language keywords, types, built-in functions, operators, etc.
 * Keep this file updated as the language evolves — all other features
 * (linting, hover, completions) derive from this single source of truth.
 */

/** All DictatorScript keywords grouped by category. */
export const DS_KEYWORDS = {
    /** Type keywords */
    types: ['int', 'float', 'double', 'char', 'string', 'bool', 'void', 'map'] as const,

    /** Declaration keywords */
    declarations: ['declare', 'elite', 'law', 'command'] as const,

    /** Entry point */
    entryPoint: ['regime'] as const,

    /** Boolean literals */
    booleans: ['loyal', 'traitor'] as const,

    /** Control flow */
    controlFlow: ['interrogate', 'otherwise', 'impose', 'from', 'forever', 'silence', 'proceed', 'report'] as const,

    /** I/O */
    io: ['broadcast', 'demand'] as const,

    /** Memory management */
    memory: ['summon', 'kill', 'null'] as const,

    /** Module system */
    imports: ['import'] as const,
} as const;

/** Flat set of all keywords for fast lookup. */
export const ALL_KEYWORDS: ReadonlySet<string> = new Set(
    Object.values(DS_KEYWORDS).flatMap(group => [...group])
);

/** Built-in methods on arrays, strings, maps, and chars. */
export const BUILTIN_METHODS: Record<string, { appliesTo: string; description: string; signature: string }> = {
    'add':         { appliesTo: 'arrays',  description: 'Append an element to the array (push_back).',           signature: '.add(element)' },
    'remove':      { appliesTo: 'arrays/maps', description: 'Remove element by index (array) or key (map).', signature: '.remove(index_or_key)' },
    'size':        { appliesTo: 'arrays/strings/maps', description: 'Returns the number of elements.',       signature: '.size()' },
    'has':         { appliesTo: 'maps',    description: 'Check if a key exists in the map.',                     signature: '.has(key)' },
    'contains':    { appliesTo: 'strings', description: 'Check if a substring exists within the string.',        signature: '.contains(substring)' },
    'toUppercase': { appliesTo: 'chars/strings', description: 'Convert to uppercase.',                           signature: '.toUppercase()' },
    'toLowercase': { appliesTo: 'chars/strings', description: 'Convert to lowercase.',                           signature: '.toLowercase()' },
};

/** Keyword hover documentation. Maps keyword -> markdown description. */
export const KEYWORD_DOCS: Record<string, string> = {
    // Types
    'int':         '`int` — 32-bit signed integer type.',
    'float':       '`float` — Single-precision floating-point type.',
    'double':      '`double` — Double-precision floating-point type.',
    'char':        '`char` — Single character type.',
    'string':      '`string` — String type (maps to `std::string`).',
    'bool':        '`bool` — Boolean type. Values: `loyal` (true), `traitor` (false).',
    'void':        '`void` — No return value.',
    'map':         '`map<K, V>` — Hash map type (maps to `std::unordered_map<K, V>`).',

    // Declarations
    'declare':     '`declare` — Declare a variable.\n\n```ds\ndeclare int x = 5\n```',
    'elite':       '`elite` — Const modifier. The variable cannot be reassigned.\n\n```ds\ndeclare elite string name = "Z"\n```',
    'law':         '`law` — Define a struct.\n\n```ds\nlaw Point {\n    int x\n    int y\n}\n```',
    'command':     '`command` — Define a function.\n\n```ds\ncommand add(int a, int b) -> int {\n    report a + b\n}\n```',

    // Entry point
    'regime':      '`regime start` — Program entry point (maps to `main()`).\n\n```ds\nregime start {\n    broadcast("Hello\\n")\n}\n```',

    // Booleans
    'loyal':       '`loyal` — Boolean `true` literal.',
    'traitor':     '`traitor` — Boolean `false` literal.',

    // Control flow
    'interrogate': '`interrogate` — If statement.\n\n```ds\ninterrogate (x > 0) {\n    broadcast("positive\\n")\n}\n```',
    'otherwise':   '`otherwise` — Else / else-if branch.\n\n```ds\n} otherwise interrogate (x < 0) {\n    ...\n} otherwise {\n    ...\n}\n```',
    'impose':      '`impose` — Loop construct (for-range, for-each, while, forever).\n\n```ds\nimpose (int i from [0..10]) { }\nimpose (condition) { }\nimpose forever { }\n```',
    'from':        '`from` — Used in `impose` loops to specify the source.\n\n```ds\nimpose (int i from [1..5]) { }\nimpose (int x from arr) { }\n```',
    'forever':     '`forever` — Infinite loop modifier.\n\n```ds\nimpose forever {\n    // runs indefinitely\n}\n```',
    'silence':     '`silence` — Break out of a loop (equivalent to `break`).',
    'proceed':     '`proceed` — Skip to next iteration (equivalent to `continue`).',
    'report':      '`report` — Return a value from a function.\n\n```ds\nreport a + b\n```',

    // I/O
    'broadcast':   '`broadcast(expr)` — Print to stdout.\n\n```ds\nbroadcast("Hello " + name + "\\n")\n```',
    'demand':      '`demand(var)` — Read input from stdin.\n\n```ds\ndeclare string name\ndemand(name)\n```',

    // Memory
    'summon':      '`summon Type` — Allocate memory on the heap (equivalent to `new`).\n\n```ds\ndeclare int-> ptr = summon int\n```',
    'kill':        '`kill expr` — Free heap memory (equivalent to `delete`).\n\n```ds\nkill ptr\n```',
    'null':        '`null` — Null pointer literal (equivalent to `nullptr`).',

    // Imports
    'import':      '`import "filename"` — Import declarations from another `.ds` file.\n\n```ds\nimport "utils"\n```',

    // Special context word
    'start':       '`start` — Used after `regime` to mark the entry point.',
};

/** Escape sequences valid in string and char literals. */
export const VALID_ESCAPE_SEQUENCES = ['\\n', '\\t', '\\\\', '\\"', "\\'", '\\0'] as const;
