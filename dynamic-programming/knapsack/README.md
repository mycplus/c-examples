# Knapsack Problem in C

Companion code for [Knapsack Problem in C and C++: 0/1 Dynamic Programming, Item Recovery and the Fractional Variant](https://www.mycplus.com/computer-science/algorithms/solving-the-knapsack-problem/) on MYCPLUS.

| File | What it is |
| --- | --- |
| `pitfalls/stack_table.c` | the full table as a variable-length array on the stack: correct for small inputs, a stack overflow at 100 items and capacity 100,000 |
| `pitfalls/upward_and_greedy.c` | capacities visited upwards (which solves unbounded, not 0/1, knapsack), greedy by value per unit weight, and the table size when weights and capacity are scaled |
| `src/knapsack_01.c` | 0/1 knapsack in C11: `knapsack_value` (one row, O(W) memory) and `knapsack_select` (also reports the chosen items, one byte per table cell) |
| `src/knapsack_fractional.c` | fractional knapsack, greedy by value per unit weight |
| `tests/compare_output.cmake` | runs a program and compares its output with a file in `expected/`, ignoring carriage returns |
| `tests/test_knapsack.c` | edge cases (no items, zero capacity, zero-weight items, negative input, value overflow) and 20,000 random instances checked against exhaustive search |

```
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

`tests/test_knapsack.c` includes `src/knapsack_01.c` directly and renames its `main`,
so the article's listing stays one complete program.
The programs in `pitfalls/` are deliberately wrong; the article explains what each one does.
`upward_and_greedy.c` is built and its output pinned by the tests. `stack_table.c` uses a
variable-length array, which MSVC does not support, so it is not built by CMake; the workflow
checks that AddressSanitizer reports its stack overflow instead.
