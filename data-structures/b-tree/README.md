# B-Tree in C

[![B-Tree](https://github.com/mycplus/c-examples/actions/workflows/b-tree.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/b-tree.yml)

Companion code for [B-Tree in C](https://www.mycplus.com/computer-science/data-structures/b-tree/) on MYCPLUS: a B-tree of 64-bit keys and values with any minimum degree, insertion and deletion in one pass down the tree, range queries, and a checker for every B-tree invariant.

| File | What it is |
| --- | --- |
| `src/btree.h`, `src/btree.c` | Create, insert (or update), search, remove, range, walk, statistics, `btree_check()` and `btree_print()` |
| `src/btree_demo.c` | Builds a small 2-3-4 tree, prints it level by level, searches, removes and runs a range query |
| `src/btree_shape.c` | Height, node count, fill and nodes read per search for 1,000,000 keys in random and ascending order |
| `tests/test_btree.c` | Unit tests, and model-based tests: random operations checked against a sorted array, with every invariant checked after every operation |
| `tests/mutations.py` | Nine deliberate mistakes, each built with sanitizers and run against the tests |
| `bench/btree_speed.c` | Insert, search and remove speed for minimum degrees 2 to 256, with a sorted-array baseline |
| `run_all.sh`, `run_all.ps1` | Every check in one command, for Ubuntu/WSL and for Windows (MSVC) |

## Build and run

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/btree_demo
./build/btree_shape
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 on Linux and with MSVC `/W4 /WX` on Windows, warnings as errors.
- 12 model-based runs (minimum degrees 2, 3, 4, 5, 8 and 32; up to 200,000 random operations each) agree with a sorted-array model, and every B-tree invariant holds after each checked operation; each tree is then emptied in random order.
- The demo and the shape statistics print exactly the files in `tests/expected/`.
- On Linux: everything runs clean under AddressSanitizer and UndefinedBehaviorSanitizer, and each of the nine mutations in `tests/mutations.py` makes the tests fail.
