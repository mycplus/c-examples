# Merge Sort in C

Companion code for [Merge Sort in C, C++, Java, Python and C#](https://www.mycplus.com/computer-science/algorithms/merge-sort/) on MYCPLUS.

| File | What it is |
| --- | --- |
| `pitfalls/base_case.c` | with half-open ranges [lo, hi), "stop when lo >= hi" never stops on a one-element range |
| `pitfalls/malloc_per_merge.c` | allocating a temporary array inside every merge. |
| `pitfalls/midpoint_overflow.c` | (lo + hi) / 2 with int indices near INT_MAX. |
| `pitfalls/ties_right.c` | taking from the right run on ties still sorts, but it reverses the order of equal keys |
| `src/merge_count.c` | comparisons on 10,000 values, against the worst-case bound, and inversions counted by merge sort against a brute-force count. |
| `src/merge_demo.c` | merge sort on 8 elements, one level of merges per line, with the comparisons each merge makes |
| `src/merge_example.c` | arrays, a linked list, and inversion counting |
| `src/merge_sort.c` | merge sort in C11. |
| `src/merge_sort.h` | merge sort for int arrays and linked lists, and inversion counting |
| `src/merge_time.c` | median of 5 timed runs on 1,000,000 ints, random and sorted. |
| `tests/compare_output.cmake` | runs a program and compares its output with a file in `expected/`, ignoring carriage returns |
| `tests/test_merge_sort.c` | both array sorts against qsort() and the list sort against the array result, on 20,000 random inputs of 0 to 64 elements |

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
