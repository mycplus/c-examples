# Enum in C

[![enums](https://github.com/mycplus/c-examples/actions/workflows/enums.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/enums.yml)

Companion code for [Enum in C: Declaration, Values, switch, Flags and Common Mistakes](https://www.mycplus.com/programming/c/enum-in-c/) on MYCPLUS.

| File | What it is |
| --- | --- |
| `src/basics.c` | Declaring enums, default and explicit values, aliases, a count constant and looping |
| `src/underlying.c` | Which integer type the compiler chose for an enum (implementation-defined, so the output differs between compilers) |
| `src/order.h`, `src/order.c` | An online order's status as an enum: names, parsing, validation and the allowed status changes |
| `src/order_demo.c` | Applies status updates from `data/events.txt` and rejects the ones the rules forbid |
| `src/flags.c` | Permission flags: one bit per constant, combined in an `unsigned int` |
| `src/log_levels.c` | An X-macro that generates an enum and its names from one list |
| `src/c23_enum.c` | C23 enums with a fixed underlying type (`enum e : unsigned char`); built only by compilers that support it |
| `cpp/enum_class.cpp` | The same ideas in C++17: plain enums, `enum class`, and an underlying type |
| `pitfalls/out_of_range.c` | A number read from a file used as an enum without validation (`checked` / `unchecked`) |
| `pitfalls/reorder.c` | Inserting a constant in the middle of an enum whose values were saved to a database |
| `pitfalls/missing_case.c` | A `switch` that misses a newly added constant |
| `compile_fail/` | Programs that must not compile: two enums with the same constant, a `const int` case label, and two C++ conversions |
| `tests/` | Unit tests, expected outputs, the compile-failure checks and mutation tests |
| `run_all.sh`, `run_all.ps1` | One-command verification for Ubuntu/WSL and Windows (MSVC); each prints a Markdown report |

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/order_demo data/events.txt
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 (and C++17) with `-Wall -Wextra -pedantic -Werror`, and with MSVC under `/W4 /WX`. `pitfalls/missing_case.c` is the one exception: its warning is the lesson, so it is allowed to build with it.
- Every program prints exactly the output shown in the article. `underlying` only has to run, because its answer is up to the compiler.
- Each program in `compile_fail/`, and `missing_case.c` built with its warning as an error, must fail to compile with a message that names the problem.
- `tests/mutations.py` puts 8 mistakes into the order library and confirms the unit tests catch each one.
- The unchecked lookup in `pitfalls/out_of_range.c` is reported by the sanitizers.
