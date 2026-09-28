# Stack in C

[![stack](https://github.com/mycplus/c-examples/actions/workflows/stack.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/stack.yml)

Companion code for [Stack Implementation in C, C++, Java, Python and C#](https://www.mycplus.com/computer-science/data-structures/stack-implementation/) on MYCPLUS.

| File | What it is |
| --- | --- |
| `src/array_stack.c` | The article's growable array-backed stack of `int`, with a bracket checker |
| `src/linked_stack.c` | The article's linked-list stack of `int` |
| `src/growth.c` | Counts resizes and element moves for four growth policies over 1,000,000 pushes |
| `src/memory.c` | Heap bytes for 1,000,000 ints in each layout (Linux/glibc only: uses `mallinfo2`) |
| `tests/` | Unit and differential tests, and the expected output of each demo |

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 with `-Wall -Wextra -pedantic -Werror`, and with MSVC under `/W4 /WX`.
- Both stacks agree with a plain-array model over 200,000 random pushes and pops, including `INT_MIN` and `INT_MAX`.
- Popping or peeking an empty stack returns `false` and leaves the output variable untouched.
- A push that would overflow the capacity arithmetic fails without calling `realloc` and leaves the stack unchanged.
- The bracket checker handles empty input, unmatched openers and closers, crossed pairs and 100,000 levels of nesting.
- A linked stack of 1,000,000 nodes is freed without recursion, with no leaks under LeakSanitizer.
- `array_stack`, `linked_stack` and `growth` print exactly the output shown in the article.
- The tests and demos run clean under AddressSanitizer and UndefinedBehaviorSanitizer.

The build does **not** check `memory`'s figures: they come from glibc's allocator and vary by platform.

The tests include the `src/` files with `main` renamed, so they test the same code the article shows rather than a copy.
