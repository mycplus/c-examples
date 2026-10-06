# Towers of Hanoi in C

[![Towers of Hanoi](https://github.com/mycplus/c-examples/actions/workflows/towers-of-hanoi.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/towers-of-hanoi.yml)

Companion code for [Towers of Hanoi: Recursive Algorithm With Code in C, C++, Java, Python and C#](https://www.mycplus.com/computer-science/algorithms/towers-of-hanoi/) on MYCPLUS.

| File | What it is |
| --- | --- |
| `src/towers_of_hanoi.c` | The article's recursive listing: prints the moves for 3 disks |
| `src/towers_of_hanoi_iterative.c` | The article's iterative listing: each move computed from the bits of its number |
| `src/hanoi.h`, `src/hanoi.c` | Recursive and iterative solvers with a callback per move, `hanoi_move_at` for any single move, and `hanoi_move_count` (up to 64 disks) |
| `src/hanoi_cli.c` | `hanoi_cli N [--iterative]` prints the moves for 0 to 20 disks and rejects anything else |
| `src/hanoi_time.c` | Median of 5 timed runs per disk count, recursive and iterative (POSIX only) |
| `pitfalls/base_case_one.c` | A base case of `n == 1`: works for 3 disks, overflows the stack for 0 |
| `pitfalls/copied_arguments.c` | Pegs passed in the wrong order: 2^n - 1 moves, illegal from move 2 |
| `pitfalls/shift_overflow.c` | `(1 << n) - 1` in `int`: undefined behaviour from 31 disks |
| `tests/test_hanoi.c` | Simulates every move for 0 to 20 disks, compares the two solvers, checks `hanoi_move_at` for 64 disks against an independent reference, and checks the 63/64/65-disk boundaries |

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The tests also compare each listing's output with `tests/expected/` and confirm the command-line program rejects malformed arguments. `hanoi_time` is built on POSIX systems but not run by the tests: timings depend on the machine. The programs in `pitfalls/` are deliberately wrong; the article explains each one, and the workflow confirms they fail the way it says.
