# Eight Queens Puzzle in C

[![Eight Queens](https://github.com/mycplus/c-examples/actions/workflows/eight-queens.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/eight-queens.yml)

Companion code for [Eight Queens Puzzle in C](https://www.mycplus.com/computer-science/algorithms/eight-queens-puzzle/) on MYCPLUS: a backtracking solver for N queens, a symmetry check that finds the distinct solutions, and an SDL2 viewer to browse them.

| File | What it is |
| --- | --- |
| `src/queens.h`, `src/queens.c` | The solver: backtracking with column and diagonal tables, a validity check and a symmetry check |
| `src/queens_cli.c` | `queens [N]`: prints the first solution and counts them all |
| `src/queens_stats.c` | Solutions, distinct solutions and search effort for 4 x 4 to 12 x 12 |
| `src/queens_view.c` | SDL2 viewer: arrow keys browse the solutions; `--screenshot K FILE` draws one to a BMP |
| `pitfalls/never_stops.c` | An iterative backtracking loop with no exit condition |
| `tests/` | Unit tests, a screenshot test, an X11 key-press test, expected outputs |

## Build and run

SDL2 is optional; without it, everything except `queens_view` builds.

```sh
sudo apt install libsdl2-dev        # Debian/Ubuntu; brew install sdl2 on macOS
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/queens 8
./build/queens_view 8
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 on Linux and with MSVC `/W4 /WX` on Windows (solver only), warnings as errors.
- Solution counts for 1 x 1 to 12 x 12 and distinct counts under rotation and reflection match OEIS A000170 and A002562, and every solution passes the validity check.
- `queens`, `queens_stats` and the pitfall print exactly the files in `tests/expected/`; bad arguments are rejected.
- The viewer's screenshot of solution 1 has a queen in exactly the right eight squares.
- On Linux, the viewer is driven with key presses in a virtual X display and its window title follows them.
- Everything runs clean under AddressSanitizer and UndefinedBehaviorSanitizer, and the project also builds and tests without SDL2.
