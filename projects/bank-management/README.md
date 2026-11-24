# Bank Management System in C

A teller's console program for a small bank: it opens savings and current accounts, takes deposits and withdrawals, moves money between accounts, posts monthly interest, and prints statements, an account list and a daily transaction report. Everything is kept in one **append-only ledger**: the program never rewrites or deletes a line, and every balance is rebuilt by replaying the ledger at start-up. Each line is checked on every start (entry numbers, dates, the bank's rules and the balance written on the line), and a ledger that breaks any rule stops the program with the file's line number instead of guessing. Money is kept in whole cents in a `long long`, never in `float` or `double`. Standard C11 only; it builds with GCC, Clang and Visual Studio.

This project goes with the article [Bank Management System in C](https://www.mycplus.com/programming/c/bank-management-system/) on MYCPLUS.

## Build

With CMake, which also builds and runs the tests:

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure
```

With GCC in one line (Linux, macOS, WSL):

```sh
gcc -std=c11 -Wall -Wextra -o bank_management src/*.c
```

With Visual Studio, from a **Developer Command Prompt**:

```bat
cl /W4 /Fe:bank_management.exe src\*.c
```

## Run

```sh
./bank_management                      # uses bank.txt and today's date
./bank_management --file mybank.txt    # another ledger
./bank_management --verify             # check the ledger and print a summary, no menu
```

`--today YYYY-MM-DD` fixes the date (the tests use it). The options may come in any order. On Windows, run `bank_management.exe` the same way.

```text
Bank Management System
======================
Today is 2026-12-01.
Loaded 3 account(s) and 14 ledger entries from bank.txt.

 1. Open an account
 2. Deposit
 3. Withdraw
 4. Transfer
 5. Account statement
 6. List all accounts
 7. Search by name
 8. Daily transaction report
 9. Post monthly interest
10. Update holder details
11. Close an account
 0. Quit
Choose an option (0-11): 5
Account number: 100001

Account 100001 (Savings, open)
Holder:  Ana Lopez
Address: 14 Harbour Road, Springfield
Opened:  2026-12-01

Date         No.  Description                     Debit       Credit        Balance
2026-12-01     1  Opening deposit                           1,500.00       1,500.00
2026-12-01     4  Cash deposit                                250.75       1,750.75
2026-12-01     5  Cash deposit                                100.00       1,850.75
2026-12-01    12  Details updated                                          1,850.75
2026-12-01    13  Interest for 2026-12                          4.63       1,855.38
Balance: 1,855.38
```

## The ledger file

Line 1 is `# MYCPLUS bank ledger v1`. Every later line is one entry, its fields separated by `|`, with money in **whole cents and no decimal point** (the file is for the program; only the screen adds commas and a point).

| Type | Fields |
|---|---|
| OPEN | `seq\|date\|OPEN\|account\|S or C\|amount\|holder\|address` |
| DEPOSIT | `seq\|date\|DEP\|account\|amount\|balance_after` |
| WITHDRAW | `seq\|date\|WDL\|account\|amount\|balance_after` |
| TRANSFER | `seq\|date\|XFER\|from\|to\|amount\|from_balance_after\|to_balance_after` |
| INTEREST | `seq\|date\|INT\|account\|YYYY-MM\|amount\|balance_after` |
| EDIT | `seq\|date\|EDIT\|account\|holder\|address` |
| CLOSE | `seq\|date\|CLOSE\|account` |

```text
# MYCPLUS bank ledger v1
1|2026-12-01|OPEN|100001|S|150000|Ana Lopez|12 Harbour Road, Springfield
2|2026-12-01|OPEN|100002|C|250050|Ben Kim|4 Mill Lane
3|2026-12-01|OPEN|100003|S|100000|Carla Diaz|9 Elm Street
4|2026-12-01|DEP|100001|25075|175075
```

`seq` runs 1, 2, 3, ... with no gaps, and each money-moving line carries the balance after it, which the loader recomputes and compares. A last line without a newline means the program stopped while writing it; the loader reports it instead of using half a line. `--verify` runs the same checks and prints, for example, `OK: 3 account(s), 14 ledger entries, last entry 2026-12-01. Money held: 6,467.38.` or `bank.txt line 7: this would take account 100002 below its limit`.

## Rules the program enforces

The menu and the loader use the same function (`bank_check`), so a hand-edited ledger cannot get past a rule the menu would enforce.

- **Savings** accounts earn 3.00% a year, paid monthly, and may not go below 0.00. **Current** accounts earn no interest and may go overdrawn down to -500.00.
- Account numbers start at 100001 and go up by 1. Closed accounts keep their numbers, so a number is never reused.
- A holder name is 2 to 30 characters and an address 1 to 50; neither may contain `|`.
- Opening an account needs a deposit of at least 1,000.00.
- Deposits, withdrawals and transfers are 0.01 to 1,000,000.00 each.
- At most 5,000.00 in cash may be withdrawn from one account per day (transfers do not count).
- A transfer is between two different open accounts, and is one ledger line, so it can never be half done.
- Monthly interest is `balance x 3.00% / 12` on the balance when it is posted, rounded half up to the cent, at most once per account per calendar month. Running it twice pays nothing the second time.
- No balance may go above 999,999,999.99.
- An account can be closed only when its balance is exactly 0.00. A closed account accepts no new transactions, but its statement can still be printed.
- Entries are never dated before the previous entry. If the computer's date is earlier than the last entry, the program refuses to start.
- Every change is written to the file first and applied in memory only after the write succeeded.

## How the code is organized

| File | What it holds |
|---|---|
| [`src/main.c`](src/main.c) | The command line, start-up checks, the menu loop and one handler per menu option |
| [`src/input.c`](src/input.c) | `read_line`, `read_int`, `read_text` (from the mini projects), `read_money`, `read_yes_no`, `read_date_or_today` |
| [`src/money.c`](src/money.c) | The `Cents` type: `money_parse`, `money_format` and `interest_for_month`, integers only |
| [`src/date.c`](src/date.c) | The `Date` type: parsing, checking, comparing, formatting and today's date |
| [`src/bank.c`](src/bank.c) | `Account`, `Entry`, `Bank`, the rules (`bank_check`) and the only code that changes balances (`bank_apply`) |
| [`src/ledger.c`](src/ledger.c) | The file format: `entry_to_line`, `line_to_entry`, `ledger_load` and `ledger_append` |
| [`src/report.c`](src/report.c) | The account list, search, statements and the daily report |
| [`tests/unit_tests.c`](tests/unit_tests.c) | Unit tests for the money, date, rule and file functions, linked against the same library |

Each `.c` file has a header with only what other files need; everything else is `static`. CMake builds every file except `main.c` into a library, `bankcore`, which both the program and the unit tests link.

## Tests

`ctest` runs:

- **`unit_tests`**: money parsing and formatting, interest rounding, dates, one case per rule, the largest possible balance, and a round trip through the file format for every entry type;
- **sessions**: typed input in, exact output compared:
  - `session` exercises every menu option, and `session_verify` checks the ledger it wrote;
  - `persist1`, `persist2` and `persist3` reuse one ledger across three runs, the last with an earlier date, which is refused;
  - `eof` ends the input halfway through opening an account, and `eof_nofile` confirms no file was created;
  - `usage` checks a bad command line;
- **16 damaged ledgers** in [`tests/data`](tests/data), each the `session` ledger with one change, checked with `--verify`: a torn last line, a gap in the numbering, a wrong balance, an overdraft, the daily limit, interest paid twice, and more.

The [workflow](../../.github/workflows/bank-management.yml) runs them with GCC and Clang (C11 and C17), under AddressSanitizer and UndefinedBehaviorSanitizer, and with Visual Studio (C11 and C17).

## Ideas to extend it

- Interest on the average daily balance, as real banks pay it, instead of the balance on the day interest is posted.
- Monthly snapshots of every balance, so start-up does not replay years of entries.
- A teller login with PINs hashed by libsodium's `crypto_pwhash`, never stored as plain text.
- Export a statement to CSV.
- A tamper-evident chain: each line carries a hash of the previous line (use an HMAC with a secret key if it must resist a deliberate edit).
- Move the ledger to SQLite and use transactions.
- File locking, so two copies of the program cannot write to the same ledger at once.
