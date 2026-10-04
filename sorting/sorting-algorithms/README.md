# Sorting Algorithms in C

Seven sorting algorithms for `int` arrays (eight functions: quicksort appears with two
pivot choices), with programs that count comparisons, check stability and time each
algorithm. Each function is the C code from its own MYCPLUS article; see the table below. Companion code for
[Sorting Algorithms in C and C++](https://www.mycplus.com/computer-science/algorithms/sorting-algorithms/).

| File | What it does |
| --- | --- |
| `src/sorting.c`, `src/sorting.h` | insertion, selection, bubble, Shell (Knuth gaps), merge, heap, quicksort (Hoare, middle pivot) and quicksort (Lomuto, last pivot) |
| `src/sort_count.c` | comparisons made on 10,000 elements in six arrangements; built twice, with merge sort's skip-if-ordered check on and off (`MERGE_SKIP_SORTED`) |
| `src/stability.c` | whether each algorithm keeps equal keys in input order |
| `src/sort_time.c` | median of 5 timings at n = 20,000 and n = 1,000,000 (POSIX only) |
| `tests/test_sorting.c` | every algorithm against `qsort()` on 20,000 random arrays of 0-64 elements |
| `tests/check_article_code.py` | checks that the insertion, selection, bubble, merge and heap sort functions are still identical to the per-article folders in `../` |

Every comparison in `sorting.c` goes through the `SORT_LESS(x, y)` macro. `sort_count.c`
and `stability.c` redefine it and `#include "sorting.c"` directly, so the code they test is
the same code the library build compiles.

```
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The tests compare `sort_count`, `sort_count_noskip` and `stability` output byte for byte with
`tests/expected/`.

## Where each function comes from

| Function | Article | Folder in this repository |
| --- | --- | --- |
| `insertion_sort` | [Insertion sort](https://www.mycplus.com/computer-science/algorithms/insertion-sort/) | `../insertion-sort/` |
| `selection_sort` | [Selection sort](https://www.mycplus.com/computer-science/algorithms/selection-sort/) | `../selection-sort/` |
| `bubble_sort` | [Bubble sort](https://www.mycplus.com/computer-science/algorithms/bubble-sort/) | `../bubble-sort/` |
| `shell_sort` | [Shell sort](https://www.mycplus.com/computer-science/algorithms/shell-sort-algorithm/) (Knuth gaps, without the gap printing) | none |
| `merge_sort` | [Merge sort](https://www.mycplus.com/computer-science/algorithms/merge-sort/) | `../merge-sort/` |
| `heap_sort` | [Heap sort](https://www.mycplus.com/computer-science/algorithms/heap-sort/) | `../heap-sort/` |
| `quick_sort`, `quick_sort_last` | [Quicksort](https://www.mycplus.com/computer-science/algorithms/quicksort-algorithm/) (`quicksort_hoare`, `quicksort_safe`) | none |

Only the comparisons differ from the article code: they go through `SORT_LESS`.
`check_article_code.py` runs in CI and fails if a copied function and its source drift apart.
`sort_time` is built but not run by the tests: timings depend on the machine.
