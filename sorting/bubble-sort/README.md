# Bubble sort in C

[![bubble-sort](https://github.com/mycplus/c-examples/actions/workflows/bubble-sort.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/bubble-sort.yml)

Companion code for [Bubble Sort in C and C++](https://www.mycplus.com/computer-science/algorithms/bubble-sort/) on MYCPLUS.

| File | What it is |
| --- | --- |
| `src/bubble_sort.c` | The article's C program: bubble sort with a last-swap boundary |
| `src/bubble_sort_flag.c` | The classic swapped-flag version (function only) |
| `src/bubble_trace.c` | Prints every comparison and swap for the pass table |
| `src/bubble_count.c` | Counts comparisons and swaps for four sorts on five input shapes |
| `src/bubble_time.c` | Times bubble sort against `qsort()` (POSIX only: uses `clock_gettime`) |
| `pitfalls/` | Four deliberately broken programs from the article. **Do not copy them.** |
| `tests/` | Differential test against `qsort()`, flag-vs-boundary test, expected outputs |

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 with `-Wall -Wextra -pedantic -Werror`, and with MSVC under `/W4 /WX`.
- `bubble_sort()` agrees with `qsort()` on 20,000 random arrays of 0 to 64 elements, including `INT_MIN`, `INT_MAX` and heavy duplication.
- The swapped-flag version agrees with the boundary version on 10,000 arrays.
- `bubble_sort`, `bubble_trace` and `bubble_count` print exactly the output shown in the article.
- The test programs run clean under AddressSanitizer and UndefinedBehaviorSanitizer.
- Each pitfall program is caught: ASan reports the two out-of-bounds reads, UBSan reports the comparator overflow, and the `<=` version is shown to reorder equal keys and, in a `do/while(swapped)` loop, never to settle on duplicates.

The build does **not** run `bubble_time`: timings depend on the machine and are not a pass/fail property.

The tests include `src/bubble_sort.c` with `main` renamed, so they test the same file the article shows rather than a copy.
