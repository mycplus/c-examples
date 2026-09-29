# Brick breaker game in C

[![brick-game](https://github.com/mycplus/c-examples/actions/workflows/brick-game.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/brick-game.yml)

Companion code for [Brick Breaker Game in C and C++](https://www.mycplus.com/programming/c/brick-game/) on MYCPLUS. A Breakout-style game that runs in a terminal on Windows 10+, Linux and macOS.

| File | What it is |
| --- | --- |
| `include/breakout.h`, `src/breakout.c` | The rules: movement, collisions, scoring, the board as text. No I/O |
| `include/screen.h`, `src/screen.c` | Redraws only the characters that changed between two frames |
| `include/term.h`, `src/term.c` | Keyboard, output and clock for Windows consoles and POSIX terminals |
| `src/main.c` | The game loop |
| `include/autopilot.h`, `src/autopilot.c` | A simple computer player for the demo and the tests |
| `src/autoplay.c` | Plays a whole game without a terminal and reports the output cost |
| `src/tick_timing.c` | Compares a fixed-delay loop with a deadline loop (takes about 20 s) |
| `tests/` | Rule tests, a screen test with a small terminal emulator, a pseudo-terminal play test, expected output |

## Build and play

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/brick advanced        # novice (default), advanced or expert
```

Controls: Left/Right arrows or A/D, P or Space to pause, Q or Ctrl+C to quit.

## What the build checks

- Compiles with GCC and Clang under C11 and C17 on Linux, with Clang on macOS, and with MSVC `/W4 /WX` on Windows.
- The rules pass unit tests for walls, bricks, the corner case, paddle aiming, lost balls, game over and winning.
- Two autopilot games and 300 random-input games keep the ball out of every solid cell and the counters in step with the board, tick by tick.
- Applying `screen_diff()` output to a small terminal emulator reproduces every frame of a full game.
- `autoplay` prints exactly the output in `tests/expected/autoplay.txt`, which the C++ version shares.
- On Linux and macOS, `brick` refuses to start without a terminal, and `tests/play_in_pty.py` plays it in a pseudo-terminal, then checks that Q and Ctrl+C both exit with status 0 and restore the cursor.
- Tests and `autoplay` run clean under AddressSanitizer and UndefinedBehaviorSanitizer.

The build does not run the interactive game on Windows and does not check `tick_timing`'s figures, which depend on the machine.
