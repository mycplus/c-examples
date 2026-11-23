# Profit and Loss Calculator in C

A menu-driven calculator for a small shop owner or a commerce student. It works out the profit or loss on a sale (with markup and margin percentages), the selling price that gives a target profit or loss, and the break-even quantity that covers fixed costs. Money is stored as whole cents in a `long long`, so totals are exact, and every amount is parsed by hand so that a third decimal place, a sign or a comma is rejected instead of silently rounded.

This project is part of [C Mini Projects for Beginners](https://www.mycplus.com/programming/c/c-mini-projects/#profit-loss-calculator) on MYCPLUS.

## Build

With GCC or Clang (Linux, macOS, WSL):

```sh
gcc -std=c11 -Wall -Wextra -o profit_loss_calculator profit_loss_calculator.c
```

With Visual Studio, from a **Developer Command Prompt**:

```bat
cl /W4 profit_loss_calculator.c
```

No `-lm` is needed: rounding is done by a small helper instead of `llround()`.

## Run

```sh
./profit_loss_calculator        # Linux, macOS, WSL
profit_loss_calculator.exe      # Windows
```

To replay the test session: `./profit_loss_calculator < tests/session.txt`.

## Sample session

```text
Profit and Loss Calculator
==========================

1. Profit or loss on a sale
2. Selling price for a target profit or loss
3. Break-even quantity
0. Quit
Choose an option (0-3): 1
Cost price per unit: 12.50
Selling price per unit: 15
Quantity sold: 100
Total cost:     1250.00
Total revenue:  1500.00
Result:         Profit of 250.00
Markup (profit on cost):            20.00%
Margin (profit on selling price):   16.67%

1. Profit or loss on a sale
2. Selling price for a target profit or loss
3. Break-even quantity
0. Quit
Choose an option (0-3): 0

Goodbye!
```

## What you practice

- `if` / `else if` / `else` with three outcomes: profit, loss, or neither
- Money as whole cents in a `long long`, so 12.50 x 100 is exactly 1250.00; `double` only for percentages
- Turning text into a number by hand in `parse_money()` with `value * 10 + (c - '0')`, and why `isdigit()` needs an `(unsigned char)` cast
- Guarding against division by zero: a percentage of a zero cost price is "not defined", and break-even is impossible when a unit earns nothing
- Markup (profit on cost) vs margin (profit on selling price)
- Rounding to the nearest cent with a one-line helper, so the build needs no `-lm`
- Ceiling division with integers: `(a + b - 1) / b` for "how many whole units are needed"
- Overflow limits: amounts up to 1,000,000,000.00 and quantities up to 1,000,000 keep every total below 10^17 cents

## Tests

`tests/session.txt` and `tests/eof.txt` are typed input; the matching `.expected.txt` files are the exact output. They run with `ctest` from the [`mini-projects`](../) folder.

## Ideas to extend it

- Add a discount or sales-tax step before the profit calculation.
- Accept thousands separators (`1,250.00`) in `parse_money()`.
- Print a table of profit at quantities 10, 20, ... 100.
