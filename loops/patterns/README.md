# Pattern Programs in C

[![patterns](https://github.com/mycplus/c-examples/actions/workflows/patterns.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/patterns.yml)

Companion code for [Pattern Programs in C: Star, Pyramid, Diamond and Number Patterns](https://www.mycplus.com/programming/c/pattern-programs-in-c/) on MYCPLUS. The same patterns in Java are in [mycplus/java-examples](https://github.com/mycplus/java-examples/tree/main/loops/patterns), and both print identical output.

| File | What it is |
| --- | --- |
| `src/pyramid.c` | A complete beginner program: reads the number of rows, checks it, prints a pyramid |
| `src/patterns.h`, `src/patterns.c` | 16 patterns, each a function that writes whole lines with no trailing spaces |
| `src/patterns_cli.c` | `patterns NAME ROWS` prints any of them; `patterns list` names them all |
| `src/shapes.c` | A second technique: test every cell of a grid against a rule such as \|x\| + \|y\| <= r |
| `src/barchart.c` | A real-world use: a vertical bar chart drawn from `data/downloads.txt` |
| `pitfalls/` | Off-by-one stars, a doubled diamond middle, trailing spaces, an unchecked `scanf`, and Pascal's triangle from factorials |
| `tests/` | Rule-based unit tests for every pattern at every size, expected outputs, and mutation tests |
| `run_all.sh`, `run_all.ps1` | One-command verification for Linux and Windows (MSVC) |

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/patterns diamond 5
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 with `-Wall -Wextra -pedantic -Werror`, and with MSVC under `/W4 /WX`.
- Every pattern, at every size it allows, is checked against rules for its shape: row count, leading spaces, star count per row, symmetry, no trailing spaces. Pascal's triangle is checked entry by entry, and `pascal_entry()` against the addition rule up to row 67, the last row that fits in 64 bits.
- Every program prints exactly the output shown in the article.
- `tests/mutations.py` puts 10 mistakes into `patterns.c` and confirms the tests catch each one.
