# Arrays in C

[![arrays-in-c](https://github.com/mycplus/c-examples/actions/workflows/arrays-in-c.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/arrays-in-c.yml)

Companion code for [Arrays in C: Declare, Initialize, Loop and Pass Arrays to Functions](https://www.mycplus.com/programming/c/arrays/) on MYCPLUS. For the array as a data structure (insertion, deletion, searching and their cost), see [Array Operations](../array-operations/).

| File | What it is |
| --- | --- |
| `src/declare.c` | Declaring and initializing arrays, including designated initializers |
| `src/layout.c` | How the elements sit next to each other in memory |
| `src/grades.c` | Total, average, best and worst of a student's test scores |
| `src/functions.c` | A week of temperatures passed to functions: average, conversion in place, days above a limit |
| `src/copy_compare.c` | Copying with `memcpy` and comparing with `memcmp` |
| `src/playlist.c` | Reversing a playlist in place |
| `src/seating.c` | A cinema seat map as a two-dimensional array |
| `src/row_major.c` | How a two-dimensional array is laid out in memory |
| `pitfalls/` | Deliberately broken programs: reading past the end, assigning arrays with `=`, comparing arrays with `==` |
| `tests/` | Unit tests, the expected output of every demo, and a check that each pitfall still fails as described |
| `run_all.sh`, `run_all.ps1` | One-command verification for Ubuntu/WSL and Windows (MSVC, C11 and C17); each prints a Markdown table |

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

To check everything in one go: `sh run_all.sh` on Ubuntu or WSL, `.\run_all.ps1` from a Developer PowerShell on Windows.

## What the build checks

- Compiles with GCC and Clang under C11 and C17 with `-Wall -Wextra -pedantic -Werror`, and with MSVC under `/W4 /WX` in C11 and C17.
- Every demo prints exactly the output shown in the article.
- The temperature functions handle an empty array, use a strict "above" comparison and convert 0, 100 and -40 degrees exactly; the reversal works for every length from 0 to 7; seat booking refuses taken seats and every out-of-range row and column, including negative ones.
- On Linux: assigning arrays with `=` fails to compile, comparing them with `==` draws a warning and compares addresses, and AddressSanitizer reports the out-of-bounds read.
- The tests and demos run clean under AddressSanitizer and UndefinedBehaviorSanitizer.
