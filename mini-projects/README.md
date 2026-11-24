# C Mini Projects for Beginners

Fifteen beginner C projects, each one file using only the standard library, that accompany the article [C Mini Projects for Beginners](https://www.mycplus.com/programming/c/mini-projects-in-c/) on MYCPLUS.

[![c-mini-projects](https://github.com/mycplus/c-examples/actions/workflows/c-mini-projects.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/c-mini-projects.yml)

| Project | What you practice | Folder |
| --- | --- | --- |
| Unit Converter | Menu loops, functions, lookup tables, safe number input | [unit-converter/](unit-converter/) |
| Profit and Loss Calculator | `if`/`else`, money in whole cents, parsing numbers by hand, percentages | [profit-loss-calculator/](profit-loss-calculator/) |
| Hangman | Strings, character arrays, lookup arrays, nested loops, command-line arguments | [hangman/](hangman/) |
| Swat the Fly | Your own random number generator, `uint32_t`, arrays, game state, command-line seed | [swat-the-fly/](swat-the-fly/) |
| Student Grade Report | 1D and 2D arrays, averages, highest and lowest, letter grades, formatted tables | [grade-report/](grade-report/) |
| Balloon Shooter | Structs, an array of structs, `enum`, a game loop, a 2D grid built from data | [balloon-shooter/](balloon-shooter/) |
| Sliding Number Puzzle | 2D arrays in a struct, Fisher-Yates shuffle, inversions and solvability, legal moves | [sliding-puzzle/](sliding-puzzle/) |
| Fly Away: Airport Text Adventure | A data-driven world, `enum`s, designated initializers, parsing commands, game state in a struct | [fly-away-adventure/](fly-away-adventure/) |
| Mini Piano | `math.h`, generating a sine wave, writing a binary WAV file byte by byte, `strtok` | [mini-piano/](mini-piano/) |
| Personal Diary | Text files, file modes, `errno`, dates with `time.h`, leap years, case-insensitive search | [personal-diary/](personal-diary/) |
| Address Book | Arrays of structs, `qsort` and comparison functions, CSV files, a safe save, `memmove` | [address-book/](address-book/) |
| Birthday Reminder | Date arithmetic, a `Date` struct by value, leap years, sorting by a calculated key, reuse | [birthday-reminder/](birthday-reminder/) |

## Build one project

Each project is a single `.c` file, so one compiler command is enough:

```sh
gcc -std=c11 -Wall -Wextra -o unit_converter unit-converter/unit_converter.c
```

```bat
cl /W4 unit-converter\unit_converter.c
```

## Build and test everything

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

With Visual Studio, add `--config Release` to the build and `-C Release` to `ctest`.

Each project has session tests: a file of typed input in `tests/`, and the exact output it must produce. Warnings are errors (`-Wall -Wextra -pedantic -Werror`, MSVC `/W4 /WX`).

To run the full matrix and print a results table:

- Ubuntu or WSL (GCC and Clang, C11 and C17, plus AddressSanitizer and UBSan): `sh run_all.sh`
- Windows (MSVC, C11 and C17): `powershell -ExecutionPolicy Bypass -File run_all.ps1`
