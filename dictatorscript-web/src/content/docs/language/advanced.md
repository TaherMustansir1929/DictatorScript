---
title: Advanced Features
description: Concurrency and Structured Bindings.
---

Because DictatorScript transpiles to C++, it inherits many powerful modern C++ features out of the box.

## Concurrency

You can easily spawn new threads using the `spawn` keyword, which translates directly to `std::thread`.

```dictatorscript
command workerTask(int id) -> void {
    broadcast("Thread " + id + " running...\n")
}

regime start {
    declare auto t1 = spawn workerTask(1)
    declare auto t2 = spawn workerTask(2)
    
    // Wait for threads to finish
    t1.join()
    t2.join()
    
    broadcast("All tasks completed.\n")
}
```

## Structured Bindings

DictatorScript supports structured bindings, allowing you to unpack data structures like structs or pairs directly into variables using the `unpack` keyword. This maps to `auto [a, b] = ...` in C++17.

```dictatorscript
law Pair {
    int first
    int second
}

regime start {
    declare Pair coords
    coords.first = 10
    coords.second = 20

    unpack [x, y] = coords

    broadcast("x = " + x + "\n")
    broadcast("y = " + y + "\n")
}
```

These features make DictatorScript surprisingly powerful, combining simple syntax with the robust capabilities of modern C++.
