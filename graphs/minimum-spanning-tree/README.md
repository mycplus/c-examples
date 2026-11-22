# Minimum Spanning Tree, Step by Step

[![Minimum Spanning Tree](https://github.com/mycplus/c-examples/actions/workflows/minimum-spanning-tree.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/minimum-spanning-tree.yml)

Companion code for [Breaking Down Complex Algorithms: A Beginner's Guide](https://www.mycplus.com/computer-science/data-structures/breaking-down-complex-algorithms-a-beginners-guide/) on MYCPLUS. The guide breaks one complex problem, connecting six offices with the cheapest set of cables, into small pieces: a brute force, a sort, a disjoint-set (union-find) structure and Kruskal's greedy loop, each tested on its own.

| File | What it is |
| --- | --- |
| `src/mst.h`, `src/mst.c` | `ds_init()`, `ds_find()`, `ds_union()`, `sort_edges()`, `kruskal()` and `mst_brute_force()` |
| `src/mst_demo.c` | The worked example: the brute force, Kruskal's decisions edge by edge, and how the work grows |
| `cpp/mst.cpp` | The same pieces in C++17 (`std::sort`, a `DisjointSet` class, `std::optional`), with a self-test |
| `pitfalls/no_cycle_check.c` | Taking the n - 1 cheapest edges without checking for cycles |
| `pitfalls/subtract_compare.c` | A `qsort` comparator written as a subtraction, which overflows |
| `pitfalls/naive_union_find.c` | Union-find without union by size or path halving: links followed |
| `tests/test_mst.c` | The pieces one by one, 5,000 small graphs against the brute force, 300 larger ones against Prim's algorithm |
| `tests/mutations.py` | Eight deliberate mistakes (and one harmless change), each run against the tests |
| `bench/mst_speed.c` | Time spent sorting against time spent in union-find, up to a million vertices |
| `run_all.sh`, `run_all.ps1` | Every check in one command, for Ubuntu/WSL and for Windows (MSVC); builds go to a temporary directory |

## Build and run

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/mst_demo
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 (with C++17 alongside) on Linux and with MSVC `/W4 /WX` on Windows, warnings as errors.
- The pieces each pass their own checks; Kruskal's algorithm matches the brute force on 5,000 small random graphs (negative weights, repeated edges, loops and disconnected graphs included) and Prim's algorithm on 300 larger ones; the C++ version matches its own brute force on 3,000 graphs.
- The demo, the C++ program and the pitfalls print exactly the files in `tests/expected/`.
- On Linux: everything runs clean under AddressSanitizer and UndefinedBehaviorSanitizer, UBSan stops the subtracting comparator, and each mutation in `tests/mutations.py` is caught.
