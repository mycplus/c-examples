# Fly Away: Airport Text Adventure in C

A small text adventure. You are at the airport drop-off and flight MC101 to Lisbon leaves in 15 minutes. Find your way through check-in, security and the departures hall to the right gate, and board before the gate closes. You type short commands such as `n`, `take passport` or `board`. Every action takes a minute, even one that fails, while looking around, checking your bag and asking for help are free, so it pays to think before you walk. There is no randomness: the same commands always give the same game.

This project is part of [C Mini Projects for Beginners](https://www.mycplus.com/programming/c/mini-projects-in-c/#fly-away-adventure) on MYCPLUS.

## Build

With GCC or Clang (Linux, macOS, WSL):

```sh
gcc -std=c11 -Wall -Wextra -o fly_away_adventure fly_away_adventure.c
```

With Visual Studio, from a **Developer Command Prompt**:

```bat
cl /W4 fly_away_adventure.c
```

## Run

```sh
./fly_away_adventure        # Windows: fly_away_adventure.exe
```

Type `help` in the game for the list of commands.

## Sample: the start and the first two commands

```text
Fly Away
========
Your flight MC101 to Lisbon leaves in 15 minutes. Get to the gate and board!
Every action takes a minute. Looking around, checking your bag and help are free.
Type help for the commands.

== Taxi drop-off ==
Your taxi is about to pull away. The terminal doors are to the north.
Exits: north
You see: passport
[15 min] > look

== Taxi drop-off ==
Your taxi is about to pull away. The terminal doors are to the north.
Exits: north
You see: passport
[15 min] > N

== Check-in hall ==
A check-in agent waves you over. Security is to the north.
Exits: north, south
You see: boarding pass
```

## What you practice

- A data-driven world: the map is a `const` table of `Room` structs, each with an `exits[4]` array indexed by a `Direction` enum, so adding a room means adding a row
- `enum` everywhere it fits: rooms, directions, items, commands and the game status, used in `switch` statements and as array indexes
- Designated initializers (`[CHECK_IN] = { ... }`, `[NORTH] = SECURITY`), and why making `NOWHERE` equal to 0 means a missing exit needs no extra code
- Game state in one struct: the current room, the minutes left, where each item is and the status
- Parsing a command: lowercasing with `tolower((unsigned char)c)`, splitting a verb from a noun, and comparing words with `strcmp`, never `==`
- Functions that return their cost in minutes, so the time rule lives in one place in `main()`
- Game rules as small `if` checks inside the command handlers

## Tests

`tests/win.txt`, `tests/lose.txt` and `tests/eof.txt` are typed commands; the matching `.expected.txt` files are the exact output. They run with `ctest` from the [`mini-projects`](../) folder. (They also contain a full playthrough, so skip them if you want to solve it yourself.)

## Ideas to extend it

- A lost-luggage side quest.
- A `map` command that draws the rooms.
- Save and load the game to a file.
- Read the rooms from a data file instead of the table.
