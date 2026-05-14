---
title: First Program
description: Write your first DictatorScript program.
---

Now that you have the `dsc` compiler built, let's write the classic "Hello, World!" program in DictatorScript.

## Hello, World!

Create a file named `hello_world.ds` and add the following code:

```dictatorscript
regime start {
    broadcast("Hello, World!\n")
}
```

### Breaking it down

- `regime start`: Every DictatorScript program must have exactly one `regime start` block. This is the entry point of your program, equivalent to the `main()` function in C++ or C.
- `broadcast()`: This is the built-in command used to print output to the console. It works similar to `std::cout` in C++ or `print()` in Python.
- `\n`: A newline character to ensure the output moves to the next line.

## Compiling and Running

To compile and run your program, use the `dsc` compiler you built in the previous step:

```bash
# Assuming dsc is in your PATH or you are running it from the build directory
./dsc hello_world.ds -o hello
./hello
```

You should see the output:

```text
Hello, World!
```

Congratulations! You've written and executed your first DictatorScript program. Next, let's learn about the basic language constructs.
