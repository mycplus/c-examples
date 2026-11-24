# Birthday Reminder in C

A list of birthdays kept in a plain text file. At start-up it tells you whose birthday is today and whose is coming up in the next 7 days, with the age each person turns. The full list shows everyone, soonest birthday first, with the date it will be celebrated and how many days away it is. Add and delete save the file straight away, so nothing is ever left unsaved. People born on 29 February celebrate on 28 February in years that are not leap years. The file has one person per line, readable in any text editor:

```
1985-11-30 | Carla Diaz
1990-12-01 | Ana Lopez
```

This project is part of [C Mini Projects for Beginners](https://www.mycplus.com/programming/c/mini-projects-in-c/#birthday-reminder) on MYCPLUS.

## Build

With GCC or Clang (Linux, macOS, WSL):

```sh
gcc -std=c11 -Wall -Wextra -o birthday_reminder birthday_reminder.c
```

With Visual Studio, from a **Developer Command Prompt**:

```bat
cl /W4 birthday_reminder.c
```

## Run

```sh
./birthday_reminder                      # uses birthdays.txt and today's date
./birthday_reminder --today 2027-02-28   # try another date
./birthday_reminder --file family.txt    # a second list
```

On Windows, run `birthday_reminder.exe` the same way.

## Sample session

With three birthdays saved, on 1 December 2026:

```text
Birthday Reminder
=================
Today is 2026-12-01.
Loaded 3 birthdays from birthdays.txt.

Coming up in the next 7 days:
  today      Ana Lopez turns 36
  in 7 days  ben Kim turns 25

1. Show reminders
2. List all birthdays
3. Add a birthday
4. Delete a birthday
0. Quit
Choose an option (0-4): 2

All birthdays, soonest first:
  #  Next        Name                      Turns  When
  1  2026-12-01  Ana Lopez                    36  today
  2  2026-12-08  ben Kim                      25  in 7 days
  3  2027-02-28  Dev Patel                    23  in 89 days

1. Show reminders
2. List all birthdays
3. Add a birthday
4. Delete a birthday
0. Quit
Choose an option (0-4): 0

Goodbye!
```

## Reused from earlier projects

About a third of the program is functions copied unchanged from earlier projects:

| Function | From |
|---|---|
| `read_line`, `read_int` | [`unit_converter.c`](../unit-converter/) |
| `read_text` | [`grade_report.c`](../grade-report/) |
| `is_leap_year`, `is_valid_date` (with its `days_in_month` table inside it), `today_string`, `split_entry` | [`personal_diary.c`](../personal-diary/) |
| `compare_ignore_case` | [`address_book.c`](../address-book/) |

## What you practice

- A `Date` struct passed and returned by value, and `compare_dates()` ordering by year, then month, then day
- The day of the year from a table of days before each month, plus one after February in a leap year
- Days until the next birthday with whole-day integer arithmetic, and why not `mktime()`
- The 29 February rule in one small function, `celebration_day()`
- Sorting by a calculated key: work out each person's next birthday once, then `qsort` the results
- Breaking ties by name, ignoring case, so `ben` comes before `Zoe`
- Deleting when order does not matter: move the last element into the gap
- Saving after every change, with the same safe save as the address book
- Reusing tested functions from earlier projects

## Tests

The files in `tests/` are typed input; the matching `.expected.txt` files are the exact output. Every test fixes the date with `--today`, so the results never change. `persist1` saves three birthdays on 1 December 2026 and `persist2` loads them on 28 February 2027. They run with `ctest` from the [`mini-projects`](../) folder.

## Ideas to extend it

- Allow an unknown birth year (`--12-01`) and leave out the age.
- Show the weekday each birthday falls on.
- A `--days N` option for the reminder window.
- Import birthdays from the address book's CSV file.
