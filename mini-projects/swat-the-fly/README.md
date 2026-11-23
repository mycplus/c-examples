# Swat the Fly Game in C

A turn-based memory and arithmetic game on a wall of 9 numbered spots. Each round the program shows where a fly is sitting and lists its hops in words, such as `right, right, left`; you work out where it landed and swat that spot. A hit scores a point, and a miss costs one of your 3 lives and shows the fly's real path so you can see where you went wrong. Round 1 has 3 hops and every round adds one more, for 10 rounds. The game uses its own small random number generator, so a game number (the seed) gives the same game on every computer.

This project is part of [C Mini Projects for Beginners](https://www.mycplus.com/programming/c/mini-projects-in-c/#swat-the-fly) on MYCPLUS.

## Build

With GCC or Clang (Linux, macOS, WSL):

```sh
gcc -std=c11 -Wall -Wextra -o swat_the_fly swat_the_fly.c
```

With Visual Studio, from a **Developer Command Prompt**:

```bat
cl /W4 swat_the_fly.c
```

## Run

```sh
./swat_the_fly       # a new game every second
./swat_the_fly 42    # game number 42
```

On Windows, run `swat_the_fly.exe` the same way. The same number (any whole number from 0 to 4294967295) gives the same game on every computer and every compiler, so two players can race on the same game.

## Sample: round 1 of game 42

```text
Swat the Fly
============
Watch the fly hop along the wall, then swat the spot where it lands.
Each round it hops once more. You have 3 lives.

Round 1 of 10    Lives: 3    Score: 0

  1  2  3  4  5  6  7  8  9
  .  F  .  .  .  .  .  .  .

The fly buzzes 3 times: right, right, right
Where did it land? Swat a spot (1-9): 5
SPLAT! You got it on 5.
```

## What you practice

- Writing your own random number generator: `rand()` may differ between C libraries, so the program uses the C standard's own example generator, which gives the same game everywhere
- `uint32_t` from `<stdint.h>`: unsigned arithmetic wraps around modulo 2^32, while signed overflow is undefined behaviour
- Passing state by pointer: `uint32_t *state` goes to every function that draws a number, with no global variable
- Arrays: the fly's path is stored in `int path[MAX_HOPS + 1]` and shown again after a miss
- Keeping a value in range: a hop that would leave the wall bounces the other way
- Game state in a few variables (`round`, `lives`, `score`) and a loop that ends on either of two conditions
- Command-line arguments with validation: the optional seed is parsed with `strtoul()` and checked

## Tests

`tests/win.txt` (game 42), `tests/lose.txt` (game 7) and `tests/eof.txt` (game 42) are typed input; the matching `.expected.txt` files are the exact output. The same files pass with GCC, Clang and MSVC. They run with `ctest` from the [`mini-projects`](../) folder.

## Ideas to extend it

- A two-dimensional wall, with up and down hops.
- A "wide swatter" power-up that also hits the spots next to the chosen one.
- A high-score file.
- Hide the hop list after a few seconds (platform-specific timing, a good next step).
