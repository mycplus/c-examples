# Unit Converter in C

A menu-driven unit converter for length, mass, temperature and volume. You pick a category, a "from" unit, a "to" unit and a value; the program prints the converted value and goes back to the menu until you choose 0 or the input ends. Length, mass and volume convert through a base unit using a lookup table of exact factors, and temperature uses its own formulas through Celsius. Every number is read safely, so letters, `12abc`, `nan`, negative lengths and temperatures below absolute zero are rejected with a clear message.

This project is part of [C Mini Projects for Beginners](https://www.mycplus.com/programming/c/c-mini-projects/#unit-converter) on MYCPLUS.

## Build

With GCC or Clang (Linux, macOS, WSL):

```sh
gcc -std=c11 -Wall -Wextra -o unit_converter unit_converter.c
```

With Visual Studio, from a **Developer Command Prompt**:

```bat
cl /W4 unit_converter.c
```

## Run

```sh
./unit_converter        # Linux, macOS, WSL
unit_converter.exe      # Windows
```

To replay the test session: `./unit_converter < tests/session.txt`.

## Sample session

```text
Unit Converter
==============

1. Length
2. Mass
3. Temperature
4. Volume
0. Quit
Choose a category (0-4): 1

Length units:
  1. millimetre (mm)
  2. centimetre (cm)
  3. metre (m)
  4. kilometre (km)
  5. inch (in)
  6. foot (ft)
  7. yard (yd)
  8. mile (mi)
Convert from (1-8): 8
Convert to (1-8): 4
Value in mi: 26.2
26.2 mi = 42.1648 km

1. Length
2. Mass
3. Temperature
4. Volume
0. Quit
Choose a category (0-4): 3

Temperature units:
  1. Celsius (C)
  2. Fahrenheit (F)
  3. Kelvin (K)
Convert from (1-3): 2
Convert to (1-3): 1
Value in F: 98.6
98.6 F = 37 C

1. Length
2. Mass
3. Temperature
4. Volume
0. Quit
Choose a category (0-4): 0

Goodbye!
```

## What you practice

- A menu loop (`for (;;)`) that runs until the user quits or input ends
- Small functions with one job each: reading input, listing units, converting
- A lookup table (an array of `struct`s) instead of a long `if` chain: every linear conversion goes through the base unit, so 8 units need 8 numbers, not 56 formulas
- Temperature as the exception: different zero points mean two formulas through Celsius
- `double` arithmetic and `printf("%.6g")`, which drops trailing zeros and switches to scientific notation for very large or small results
- Safe input with `fgets()` plus `strtol()`/`strtod()`: rejecting empty lines, trailing junk, `nan`/`inf` (`isfinite()`) and overflow (`errno == ERANGE`)
- Validating against physical limits: nothing negative, nothing colder than absolute zero

## Tests

`tests/session.txt` and `tests/eof.txt` are typed input; the matching `.expected.txt` files are the exact output. They run with `ctest` from the [`mini-projects`](../) folder.

## Ideas to extend it

- Add a **speed** category (km/h, m/s, mph, knot). It needs only a new table.
- Add a command-line mode, so `unit_converter 26.2 mi km` prints the answer and exits.
- Keep a history of the last 10 conversions and add a menu option to show it.
