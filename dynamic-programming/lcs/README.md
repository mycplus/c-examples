# Longest Common Subsequence in C

[![LCS](https://github.com/mycplus/c-examples/actions/workflows/lcs.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/lcs.yml)

Companion code for [Longest Common Subsequence in C and C++](https://www.mycplus.com/computer-science/algorithms/longest-common-subsequence/) on MYCPLUS: the LCS of two strings by dynamic programming, its length in linear memory, and a line diff built on it. The C++ version is in [mycplus/cpp-examples](https://github.com/mycplus/cpp-examples/tree/main/dynamic-programming/lcs).

| File | What it is |
| --- | --- |
| `src/lcs.h`, `src/lcs.c` | `lcs_length()` (two rows), `lcs_string()` (full table and walk-back), `lcs_print_table()`, `lcs_diff()` |
| `src/lcs_demo.c` | The table for ABCBDAB and BDCABA, and the LCS of several pairs |
| `src/lcs_diff.c` | `lcs_diff OLD NEW`: a minimal line diff |
| `pitfalls/naive_recursion.c` | The recurrence as plain recursion: call counts against table cells |
| `pitfalls/free_literal.c` | A function that sometimes returns a string literal, and a caller that frees it |
| `tests/test_lcs.c` | Edge cases, 10,000 random pairs against brute force, long strings, 500 random diffs |
| `tests/compare_gnu_diff.py` | 300 random file pairs compared with GNU `diff --minimal` |
| `tests/mutations.py` | Seven deliberate mistakes (and one harmless change), each run against the tests |
| `bench/lcs_speed.c` | Time and memory of `lcs_length` and `lcs_string` up to 8,000 characters |
| `run_all.sh`, `run_all.ps1` | Every check in one command, for Ubuntu/WSL and for Windows (MSVC) |

## Build and run

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/lcs_demo
./build/lcs_diff tests/data/before.c tests/data/after.c
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 on Linux and with MSVC `/W4 /WX` on Windows, warnings as errors.
- 10,000 random pairs of short strings agree with a brute-force search; long strings, diffs and edge cases are consistent.
- The demo, the diff and the recursion pitfall print exactly the files in `tests/expected/`.
- On Linux: everything runs clean under AddressSanitizer and UndefinedBehaviorSanitizer, `free_literal` crashes as described, `lcs_diff` agrees with GNU `diff --minimal` on 300 random file pairs, and each mutation in `tests/mutations.py` is caught.
