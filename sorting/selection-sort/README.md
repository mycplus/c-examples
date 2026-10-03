# Selection Sort in C

Companion code for [Selection Sort in C, C++, Java, Python and C#](https://www.mycplus.com/computer-science/algorithms/selection-sort/) on MYCPLUS.

| File | What it is |
| --- | --- |
| `experiments/value_in_register.c` | selection sort that keeps the current minimum value in a local variable instead of re-reading a[min] on every comparison. |
| `pitfalls/exchange_sort.c` | swapping inside the inner loop still sorts, but it is not selection sort |
| `pitfalls/stability.c` | selection sort's long-distance swap reorders equal keys |
| `pitfalls/xor_swap.c` | selection sort with an XOR swap and no min != i check |
| `src/selection_count.c` | comparisons, swaps and shifts on 10,000 values, and the average swap count over 100 random permutations against n - H(n). |
| `src/selection_demo.c` | selection sort traced one pass at a time |
| `src/selection_example.c` | calls selection_sort() and its two variants |
| `src/selection_sort.c` | selection sort in C11. |
| `src/selection_sort.h` | selection sort and two variants for int arrays |
| `src/selection_time.c` | median of 5 timed runs, n = 20,000, random and sorted input. |
| `tests/compare_output.cmake` | runs a program and compares its output with a file in `expected/`, ignoring carriage returns |
| `tests/test_selection_sort.c` | all three functions against qsort() on 20,000 random arrays of 0 to 64 elements, with INT_MIN, INT_MAX and many duplicates. |

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
