# Balloon Shooter Game in C

A turn-based shooting game drawn as an 8 x 8 text grid: the sky. Balloons appear in the bottom row and rise every turn: a slow balloon (`o`) rises 1 row and a fast one (`O`) rises 2. Each turn you shoot one row, or hold fire. The arrow flies while the balloons rise, so it hits a balloon that is in your row **after** rising: aim one row above a slow balloon and two above a fast one. A balloon that rises above row 1 escapes. You have 15 arrows for 12 balloons, and the game ends when every balloon is popped or gone, or the arrows run out. The game uses its own small random number generator, so a game number gives the same game on every computer.

This project is part of [C Mini Projects for Beginners](https://www.mycplus.com/programming/c/mini-projects-in-c/#balloon-shooter) on MYCPLUS.

## Build

With GCC or Clang (Linux, macOS, WSL):

```sh
gcc -std=c11 -Wall -Wextra -o balloon_shooter balloon_shooter.c
```

With Visual Studio, from a **Developer Command Prompt**:

```bat
cl /W4 balloon_shooter.c
```

## Run

```sh
./balloon_shooter       # a new game every second
./balloon_shooter 42    # replay game 42
```

On Windows, run `balloon_shooter.exe` the same way.

## Sample: turns 1 and 2 of game 42

```text
Balloon Shooter
===============
Balloons rise from the bottom of the sky. Pop them before they escape.
o = slow balloon (rises 1 row)    O = fast balloon (rises 2 rows)
Your arrow flies while the balloons rise, so aim where a balloon WILL be.
You have 15 arrows for 12 balloons.

Turn 1    Arrows: 15    Popped: 0    Escaped: 0
 1 | . . . . . . . .
 2 | . . . . . . . .
 3 | . . . . . . . .
 4 | . . . . . . . .
 5 | . . . . . . . .
 6 | . . . . . . . .
 7 | . . . . . . . .
 8 | . O . . . . . .
Shoot which row? (1-8, 0 to hold fire): 6
POP! You hit a balloon in row 6.

Turn 2    Arrows: 14    Popped: 1    Escaped: 0
 1 | . . . . . . . .
 2 | . . . . . . . .
 3 | . . . . . . . .
 4 | . . . . . . . .
 5 | . . . . . . . .
 6 | . . . . . . . .
 7 | . . . . . . . .
 8 | . . . . . o . .
Shoot which row? (1-8, 0 to hold fire): 7
POP! You hit a balloon in row 7.
```

The fast balloon in row 8 rises to row 6 while the arrow flies, so shooting row 6 pops it. The slow balloon in turn 2 rises to row 7.

## What you practice

- A `struct` for one game object: `Balloon` holds `row`, `col`, `speed` and `active`
- An array of structs, where a popped or escaped balloon is marked inactive instead of being removed
- A `struct` that holds the whole game, so functions take one `Game *` (or `const Game *`) instead of long parameter lists
- An `enum` for the game status, and a main loop that runs `while (game_status(&game) == PLAYING)`
- Separating data from drawing: `draw()` builds a fresh `char sky[ROWS][COLS]` from the balloon list every turn
- The game loop: draw, read input, update (rise, shoot, spawn), check status, each step in its own function
- Choosing a rule that makes a bug impossible: one balloon per column means two balloons can never overlap, and `% COLS` wraps the search from column 8 back to 1
- The portable random number generator from Swat the Fly, reused, so a seed gives the same game everywhere

## Tests

`tests/win.txt` (game 42), `tests/lose.txt` (game 7) and `tests/eof.txt` (game 42) are typed input; the matching `.expected.txt` files are the exact output. The same files pass with GCC, Clang and MSVC. They run with `ctest` from the [`mini-projects`](../) folder.

## Ideas to extend it

- Balloons that drift sideways as well as up.
- A "bomb" balloon that costs an arrow if popped.
- A high-score table saved to a file.
- Real-time play with a timer (platform-specific, a good next step).
