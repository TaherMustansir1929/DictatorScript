---
title: Memory Management
description: Using raw and smart pointers in DictatorScript.
---

DictatorScript maps directly to C++ memory management principles. You can manage memory manually using raw pointers, or automatically using modern smart pointers.

## Raw Pointers

Raw pointers use familiar C++ symbols but with specific thematic keywords for allocation and deallocation.

- **Address-of:** `@` (equivalent to `&` in C++)
- **Dereference:** `^` (equivalent to `*` in C++)

```dictatorscript
declare int x = 42
declare int-> ptr = @x                  // Get the address of x
declare int val = ^ptr                  // Dereference ptr
```

### Dynamic Allocation

To allocate memory on the heap, use the `summon` keyword. To free that memory, use the `kill` keyword.

```dictatorscript
declare int-> heapVar = summon int      // new int
kill heapVar                            // delete heapVar
```

## Smart Pointers

For safer memory management without the risk of memory leaks, DictatorScript supports modern C++ smart pointers (`std::unique_ptr` and `std::shared_ptr`).

### Guard Pointers (`unique_ptr`)

A `guard` pointer owns the object it points to exclusively. Use `summon_guard` to create it.

```dictatorscript
declare guard int gPtr = summon_guard int(999)
broadcast("Guard pointer value = " + ^gPtr + "\n")
```

### Share Pointers (`shared_ptr`)

A `share` pointer allows multiple pointers to own the same object, using reference counting. Use `summon_share` to create it.

```dictatorscript
declare share int sPtr = summon_share int(7)
broadcast("Share pointer value = " + ^sPtr + "\n")
```
