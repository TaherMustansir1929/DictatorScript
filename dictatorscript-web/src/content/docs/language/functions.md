---
title: Functions and Lambdas
description: Defining commands and lambda expressions in DictatorScript.
---

Functions in DictatorScript are called **commands**. They allow you to encapsulate reusable logic.

## Defining Commands

Commands are declared using the `command` keyword, followed by the command name, parameters, and return type separated by an arrow (`->`).

Use the `report` keyword to return a value from a command (equivalent to `return`).

```dictatorscript
command add(int a, int b) -> int {
    report a + b
}

command greet(string name) -> void {
    broadcast("Hello " + name + "\n")
}
```

## Calling Commands

Commands are called just like standard functions in C++:

```dictatorscript
declare int result = add(5, 10)
greet("Citizen")
```

## Lambda Expressions

DictatorScript fully supports lambda expressions using the `block` keyword. You can store lambdas in variables using `auto` type deduction.

```dictatorscript
declare auto square = block(int n) -> int {
    report n * n
}

declare int val = square(8)  // val is 64
```

Lambdas are particularly useful for passing logic as arguments or keeping specific operations localized within a regime block.
