# Heap Sort in C

Companion code for [Heap Sort in C, C++, Java, Python and C#](https://www.mycplus.com/computer-science/algorithms/heap-sort/) on MYCPLUS.

| File | What it is |
| --- | --- |
| `pitfalls/child_bound.c` | "child <= n" instead of "child < n" in sift_down |
| `pitfalls/one_based.c` | the 1-based child formulas 2i and 2i + 1 on a 0-based array |
| `pitfalls/one_based_search.c` | how often does the 1-based child formula fail? Sorts 100,000 random arrays of 2 to 16 values in [0, 100) with it and counts the results that are not sorted. |
| `pitfalls/unsigned_build.c` | the textbook build loop with a size_t index |
| `src/heap_count.c` | comparisons to build a heap (two ways) and to sort (two variants). |
| `src/heap_demo.c` | heap sort traced |
| `src/heap_example.c` | builds a heap, then sorts, with both variants |
| `src/heap_sort.c` | heap sort in C11, with a max-heap stored in the array |
| `src/heap_sort.h` | heap sort for int arrays, and the two ways to build a heap |
| `src/heap_time.c` | heap sort, Floyd's variant and quicksort (Hoare, middle pivot) on random ints from 10^4 to 10^7 elements |
| `tests/compare_output.cmake` | runs a program and compares its output with a file in `expected/`, ignoring carriage returns |
| `tests/test_heap_sort.c` | both sorts and both heap builds against qsort() on 20,000 random arrays of 0 to 64 elements, with INT_MIN, INT_MAX and many duplicates. |

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
