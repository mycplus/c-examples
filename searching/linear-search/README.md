# Linear Search in C

[![Linear Search](https://github.com/mycplus/c-examples/actions/workflows/linear-search.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/linear-search.yml)

Companion code for [Linear Search in C and C++](https://www.mycplus.com/computer-science/algorithms/linear-search/) on MYCPLUS: the plain loop, a sentinel version, a generic search for any element type, move-to-front and transpose, and binary search for comparison. The C++ version is in [mycplus/cpp-examples](https://github.com/mycplus/cpp-examples/tree/main/searching/linear-search).

| File | What it is |
| --- | --- |
| `src/linear_search.h`, `src/linear_search.c` | `linear_search()`, `sentinel_search()`, `search_first()`, `search_move_to_front()`, `search_transpose()`, `binary_search()` |
| `src/linear_search_demo.c` | Searches for ints, a struct found by name, move-to-front step by step |
| `src/search_numbers.c` | `search_numbers FILE KEY...`: reads numbers safely and reports where each key is |
| `pitfalls/off_by_one.c` | A loop that runs to `i <= n` and finds a key that is not in the array |
| `pitfalls/unchecked_scanf.c` | Reading until 0 without checking `scanf`, given input that contains a letter |
| `pitfalls/string_equals.c` | Strings compared with `==` instead of `strcmp` |
| `tests/test_linear_search.c` | Edge cases and 20,000 random arrays against a reference |
| `tests/test_lfind.c` | 10,000 random arrays compared with POSIX `lfind()` (not built on Windows) |
| `tests/mutations.py` | Nine deliberate mistakes (and one harmless change), each run against the tests |
| `bench/comparisons.c` | Average comparisons per search, uniform and Zipf-distributed keys |
| `bench/search_speed.c` | Nanoseconds per search: linear, sentinel and binary, 4 to 4,096 elements |
| `run_all.sh`, `run_all.ps1` | Every check in one command, for Ubuntu/WSL and for Windows (MSVC) |

## Build and run

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/linear_search_demo
./build/search_numbers tests/data/numbers.txt 73 0 50
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 on Linux and with MSVC `/W4 /WX` on Windows, warnings as errors (the pitfalls' warnings are shown but allowed).
- Edge cases and 20,000 random arrays agree with a reference that scans the other way; on Linux, 10,000 random arrays agree with `lfind()`.
- The demo, `search_numbers`, the comparison counts and three pitfalls print exactly the files in `tests/expected/`.
- On Linux: everything runs clean under AddressSanitizer and UndefinedBehaviorSanitizer, the off-by-one read on the heap is caught, and each mutation in `tests/mutations.py` is caught.
