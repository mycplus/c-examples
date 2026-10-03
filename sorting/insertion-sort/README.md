# Insertion Sort in C

Companion code for [Insertion Sort in C, C++, Java, Python and C#](https://www.mycplus.com/computer-science/algorithms/insertion-sort/) on MYCPLUS.

| File | What it is |
| --- | --- |
| `pitfalls/stability.c` | "<=" instead of "<" still sorts, but reorders equal keys |
| `pitfalls/unsequenced.c` | reading and modifying j in one expression |
| `pitfalls/unsigned_index.c` | the textbook loop "j >= 0" with a size_t index |
| `src/insertion_count.c` | comparisons and moves against the inversion count. |
| `src/insertion_demo.c` | insertion sort traced step by step, plus edge cases |
| `src/insertion_example.c` | calls both functions from insertion_sort.c |
| `src/insertion_sort.c` | insertion sort in C11. |
| `src/insertion_sort.h` | insertion sort and binary insertion sort for int arrays |
| `src/insertion_time.c` | (1) n = 20,000 random ints |
| `tests/compare_output.cmake` | runs a program and compares its output with a file in `expected/`, ignoring carriage returns |
| `tests/test_insertion_sort.c` | both functions against qsort() on 20,000 random arrays of 0 to 64 elements, with INT_MIN, INT_MAX and many duplicates. |

```
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The tests compare each program's output with `tests/expected/` and check the sort
against a reference on thousands of random inputs. `*_time` programs are built on POSIX systems but not run by the tests: timings depend on the machine.
The programs in `pitfalls/` are
deliberately wrong; the article explains what each one does. Some are built and run by the
tests (their output is deterministic); the rest are checked by the workflow, which
confirms the compiler warning or sanitizer report the article describes.
