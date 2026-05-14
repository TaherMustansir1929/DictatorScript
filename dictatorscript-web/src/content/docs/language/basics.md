---
title: Language Basics
description: Learn about variables, types, and structs in DictatorScript.
---

DictatorScript has a simple, static type system inspired by C++, with a syntax designed to be more approachable.

## Variables

All variables must be declared using the `declare` keyword. If a variable is uninitialized, it is automatically zero-initialized.

```dictatorscript
declare int x = 5
declare float pi = 3.14
declare bool flag = loyal         // true
declare elite string name = "Z"   // elite means const
declare int y                     // auto-initialized to 0
```

### `auto` Type Deduction

DictatorScript supports `auto` type deduction, similar to C++11, which allows the compiler to automatically determine the type of a variable from its initializer.

```dictatorscript
declare auto answer  = 42
declare auto ratio   = 3.14
declare auto heading = "DictatorScript!"
```

## Data Types

Here is a mapping of DictatorScript data types to their C++ equivalents:

| DictatorScript | C++ Equivalent |
|---|---|
| `int` | `int` |
| `float` | `float` |
| `double` | `double` |
| `char` | `char` |
| `string` | `std::string` |
| `bool` | `bool` |
| `void` | `void` |
| `int[]` | `std::vector<int>` |
| `int->` | `int*` |
| `map<K,V>` | `std::unordered_map<K,V>` |

### Boolean Literals

DictatorScript uses thematic keywords for boolean values:
- `loyal` = `true`
- `traitor` = `false`

## Arrays and Maps

Arrays in DictatorScript map directly to `std::vector` in C++, and maps to `std::unordered_map`.

### Arrays

```dictatorscript
declare int[] nums = [1, 2, 3]
declare int[] range = [0..10]
nums.add(5)
nums.remove(0)
declare int len = nums.size()
```

### Hashmaps

```dictatorscript
declare map<string, int> scores
scores["Alice"] = 95
```

## Structs

You can define custom data structures using the `law` keyword:

```dictatorscript
law Point {
    int x
    int y
}
```
