# Pointers in C

[![pointers](https://github.com/mycplus/c-examples/actions/workflows/pointers.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/pointers.yml)

Companion code for [Pointers in C: From Addresses to Function Pointers](https://www.mycplus.com/tutorials/c-programming-tutorials/pointers/) on MYCPLUS.

| File | What it is |
| --- | --- |
| `src/basics.c` | Declaring a pointer, `&`, `*`, writing through a pointer, `NULL` |
| `src/address_demo.c` | Prints the address of a variable and the address of the pointer that holds it |
| `src/sizes.c` | `sizeof` a pointer, and how far `p + 1` moves for five element types |
| `src/arrays.c` | Array decay, `a[i]` versus `*(a + i)`, and `*q++`, `(*q)++`, `*++q` |
| `src/functions.c` | `swap`, several results through pointers (sensor statistics), and an integer parser built on `strtol` |
| `src/strings.c` | `const char *` versus `char[]`, and a word counter that walks a pointer |
| `src/const_pointers.c` | Pointer to const, const pointer, and both |
| `src/dynamic_array.c` | A growing `int` list with `realloc`, and a `calloc` histogram |
| `src/linked_list.c` | A to-do list: insert, remove and reverse using a pointer to a pointer |
| `src/callbacks.c` | Function pointers: `qsort` comparators and a calculator table |
| `src/generic_swap.c` | `void *` and a byte-wise swap that works on any type |
| `src/double_pointer.c` | A function that hands back a new allocation, and `char **argv` |
| `pitfalls/` | Deliberately broken programs: dangling pointer, use after free, double free, leak, heap overflow, NULL dereference, pointer passed by value, `sizeof` on a parameter, writing through `const`, writing to a string literal, an uninitialized pointer, `void *` misuse, and the `int* a, b` declaration trap. `run_pitfalls.sh` builds and runs them all |
| `asm/` | `sum()` written with an index and with a pointer; `compare.sh` compiles both to assembly and compares them |
| `run_all.sh`, `run_all.ps1` | One-command verification: `run_all.sh` (Ubuntu or WSL) builds with GCC and Clang, runs the tests and sanitizers and checks the pitfall diagnostics; `run_all.ps1` (Windows, MSVC, C11 and C17) builds and runs the tests. Both print a Markdown table |
| `tests/` | Unit tests, and the expected output of every demo |

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

To build, test, sanitize and check everything in one go and get a results table, run `sh run_all.sh` (Ubuntu or WSL) or `.\run_all.ps1` (Windows, from a Developer PowerShell).

The programs in `pitfalls/` are not part of the CMake build because they are meant to fail. Run them on Linux with GCC:

```sh
sh pitfalls/run_pitfalls.sh
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 with `-Wall -Wextra -pedantic -Werror`, and with MSVC under `/W4 /WX`.
- Every demo prints exactly the output shown in the article. `address_demo` prints addresses, which change on every run, so only the shape of its output is checked.
- `parse_int` rejects empty strings, trailing characters and values outside the range of `int`, and leaves its output untouched on failure.
- The growing list keeps its contents across 1,000 pushes, doubles its capacity each time, and refuses to double a capacity that would overflow `size_t`.
- The linked list agrees with a plain-array model over 20,000 random inserts, removals and reversals, and is freed without leaks.
- `qsort` with the salary comparator is ordered, with ties broken by name, over 200 random employees.
- The tests and demos run clean under AddressSanitizer and UndefinedBehaviorSanitizer.
- Each program in `pitfalls/` still produces the diagnostic the article quotes.

The build does **not** check the assembly comparison (it varies by compiler version and is printed for information only), the exact text of sanitizer reports, or behavior on 32-bit targets. `sizes` expects 8-byte pointers.

The tests include the `src/` files with `main` renamed, so they test the same code the article shows rather than a copy.
