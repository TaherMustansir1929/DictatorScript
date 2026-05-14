---
title: Control Flow
description: Conditionals and loops in DictatorScript.
---

DictatorScript offers familiar control flow structures with its unique thematic syntax.

## Conditionals

Conditional statements use the `interrogate` and `otherwise` keywords, replacing traditional `if` and `else`.

```dictatorscript
declare int x = 10

interrogate (x > 0) {
    broadcast("positive\n")
} otherwise interrogate (x < 0) {
    broadcast("negative\n")
} otherwise {
    broadcast("zero\n")
}
```

## Loops

Loops are constructed using the `impose` keyword. There are several forms of loops supported:

### Range-based Loop (Inclusive)

```dictatorscript
impose (int i from [1..5]) {
    broadcast("Count: " + i + "\n")
}
```

### For-Each Loop

```dictatorscript
declare int[] arr = [10, 20, 30]

impose (int x from arr) {
    broadcast("Value: " + x + "\n")
}
```

### While Loop

```dictatorscript
declare int count = 0

impose (count < 5) {
    broadcast("Count: " + count + "\n")
    count = count + 1
}
```

### Infinite Loop

```dictatorscript
impose forever {
    broadcast("This runs forever until silenced.\n")
    silence  // Break out of the loop
}
```

## Loop Control

To control the execution flow inside a loop, you can use:
- `silence` (equivalent to `break`): Exits the loop immediately.
- `proceed` (equivalent to `continue`): Skips the rest of the current iteration and moves to the next one.
