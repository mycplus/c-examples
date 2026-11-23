# Sliding Number Puzzle (8-Puzzle and 15-Puzzle) in C

The classic sliding-tile puzzle on a 3 x 3 board (tiles 1 to 8) or a 4 x 4 board (tiles 1 to 15). Put the tiles in order, row by row, with the gap in the bottom-right corner: type the number of a tile next to the gap to slide it in, or `0` to give up. The program counts your moves and tells you when the puzzle is solved. Easy boards are 20 random slides away from the solved board. Hard boards are a fair shuffle of every tile, and here is the catch: exactly half of all shuffles can never be solved. The program counts *inversions* to tell which half a board is in, and if it is unsolvable, swaps two tiles to move it into the solvable half.

This project is part of [C Mini Projects for Beginners](https://www.mycplus.com/programming/c/mini-projects-in-c/#sliding-puzzle) on MYCPLUS.

## Build

With GCC or Clang (Linux, macOS, WSL):

```sh
gcc -std=c11 -Wall -Wextra -o sliding_puzzle sliding_puzzle.c
```

With Visual Studio, from a **Developer Command Prompt**:

```bat
cl /W4 sliding_puzzle.c
```

## Run

```sh
./sliding_puzzle       # a new shuffle every second
./sliding_puzzle 42    # replay shuffle 42
```

On Windows, run `sliding_puzzle.exe` the same way. The same number gives the same board on every computer.

## Sample: shuffle 42, 3 x 3, easy, the first two moves

```text
Sliding Puzzle
==============
Slide the tiles into order, with the gap in the bottom-right corner.
Type the number of a tile next to the gap to slide it into the gap.
Board size (3 or 4): 3
1. Easy (20 random slides from the solved board)
2. Hard (a fair random shuffle)
Choose a difficulty (1-2): 1

Moves: 0
  4  1  3
  6  .  2
  5  7  8
Move which tile? (0 to give up): 7

Moves: 1
  4  1  3
  6  7  2
  5  .  8
Move which tile? (0 to give up): 5

Moves: 2
  4  1  3
  6  7  2
  .  5  8
Move which tile? (0 to give up): 
```

## What you practice

- A 2D array inside a struct: `Board` holds the size, the tiles (`0` is the gap), the gap's position and the move count
- Treating a 2D board as a 1D list with `index = row * size + col`, and back with `/` and `%`
- The Fisher-Yates shuffle, the fair way to shuffle an array
- Counting inversions with nested loops, and the rule that says whether a board can be solved
- Checking a move is legal: only the tiles directly above, below, left or right of the gap can move
- An `enum` for the difficulty, used in a `switch`
- Reusing earlier code: the random number generator from Swat the Fly and the input helpers from the unit converter

## Tests

`tests/win.txt` (shuffle 42, solved in the shortest possible 18 moves), `tests/giveup.txt` (shuffle 7, 4 x 4) and `tests/eof.txt` are typed input; the matching `.expected.txt` files are the exact output. The same files pass with GCC, Clang and MSVC. They run with `ctest` from the [`mini-projects`](../) folder.

## Ideas to extend it

- Accept arrow keys or `w a s d` instead of tile numbers.
- An "undo" command.
- Save the best move count for each board size.
- A solver that finds the shortest solution with breadth-first search (feasible for 3 x 3).
