# Defensive Programming in C

[![Defensive Programming](https://github.com/mycplus/c-examples/actions/workflows/defensive-programming.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/defensive-programming.yml)

Companion code for [Defensive Programming in C: 10 Techniques with Tested Examples](https://www.mycplus.com/programming/c/defensive-programming-techniques/) on MYCPLUS.

| File | What it shows |
| --- | --- |
| `src/defensive.h`, `src/defensive.c` | Checked building blocks: number parsing with `strtol`, line reading with `fgets`, bounded string copy, overflow-checked sizes and allocation, a file copy that cleans up on every error path |
| `src/people.c` | Validates a file of `name,age` records and reports each bad line with its reason |
| `tests/test_defensive.c` | 72 checks, including exact buffer-size boundaries |
| `pitfalls/` | `atoi` versus `strtol`, an `assert` with a side effect, an off-by-one `malloc`, signed overflow, a read after `free` and a leak on an error path |
| `compile_fail/` | Code that must not compile: an ignored status and a failing `static_assert` |
| `run_all.sh` | GCC and Clang (C11, C17), ASan and UBSan, Valgrind, `-fanalyzer` and four deliberate mistakes the tests must catch |
| `run_all.ps1` | MSVC (C11, C17), MSVC AddressSanitizer and the same deliberate mistakes |

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

On Windows, run these in a Developer PowerShell: the compile-fail tests call `cl` directly.
