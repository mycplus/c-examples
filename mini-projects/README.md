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
