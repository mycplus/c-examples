# Library Management System in C

A small library desk program that keeps three kinds of record that refer to each other: **books** (an ID such as 101, a title, an author and a number of copies), **members** (an ID such as 1, and a name) and **loans**. Issuing a book creates a loan, due back in 14 days; a late return is fined 0.25 per day. The loans report lists every loan, soonest due first, marking each one as due or overdue, and at start-up the program says how many loans are overdue. A loan stores only a book ID and a member ID, like a row in a database table, and the program's rules keep those links valid: no loans of books or to members that do not exist, at most 3 books per member, never more loans than copies, and no removing a member who still has books. Every change is saved straight away to one text file, one record per line, whose first letter says which kind it is:

```
B|101|2|Learning C Step by Step|Ana Writer
M|1|Ana Lopez
L|101|1|2026-12-15
```

This project is part of [C Mini Projects for Beginners](https://www.mycplus.com/programming/c/mini-projects-in-c/#library-management) on MYCPLUS.

## Build

With GCC or Clang (Linux, macOS, WSL):

```sh
gcc -std=c11 -Wall -Wextra -o library_management library_management.c
```

With Visual Studio, from a **Developer Command Prompt**:

```bat
cl /W4 library_management.c
```

## Run

```sh
./library_management                       # uses library.txt and today's date
./library_management --today 2027-01-10    # see overdue loans
./library_management --file branch2.txt    # a second library
```

On Windows, run `library_management.exe` the same way.

## Sample session

One book on loan, five days late:

```text
Library Management
==================
Today is 2026-12-20.
Loaded 1 book, 1 member and 1 loan from library.txt.
Overdue: 1 loan. Choose 7 to see them.

1. List books
2. List members
3. Add a book
4. Add a member
5. Issue a book
6. Return a book
7. Loans and overdue books
8. Remove a member
0. Quit
Choose an option (0-8): 7

1 loan, 1 overdue:
Due         Book  Title                          Member               Status
2026-12-15   101  Learning C Step by Step        Ana Lopez            overdue by 5 days

1. List books
2. List members
3. Add a book
4. Add a member
5. Issue a book
6. Return a book
7. Loans and overdue books
8. Remove a member
0. Quit
Choose an option (0-8): 6
Book ID: 101
Member ID: 1
Ana Lopez returned book 101 5 days late. Fine: 1.25

1. List books
2. List members
3. Add a book
4. Add a member
5. Issue a book
6. Return a book
7. Loans and overdue books
8. Remove a member
0. Quit
Choose an option (0-8): 5
Book ID: 101
Member ID: 1
Issued book 101 to Ana Lopez. Due back on 2027-01-03.

1. List books
2. List members
3. Add a book
4. Add a member
5. Issue a book
6. Return a book
7. Loans and overdue books
8. Remove a member
0. Quit
Choose an option (0-8): 7

1 loan, 0 overdue:
Due         Book  Title                          Member               Status
2027-01-03   101  Learning C Step by Step        Ana Lopez            due in 14 days

1. List books
2. List members
3. Add a book
4. Add a member
5. Issue a book
6. Return a book
7. Loans and overdue books
8. Remove a member
0. Quit
Choose an option (0-8): 0

Goodbye!
```

## Reused from earlier projects

| Function | From |
|---|---|
| `read_line`, `read_int` | [`unit_converter.c`](../unit-converter/) |
| `read_text` | [`grade_report.c`](../grade-report/) |
| `is_leap_year`, `is_valid_date` (with its `days_in_month` table inside it), `today_string` | [`personal_diary.c`](../personal-diary/) |
| the `Date` struct, `parse_date`, `compare_dates`, `day_of_year`, and the `days_before_month` table | [`birthday_reminder.c`](../birthday-reminder/) |

## What you practice

- Related structs linked by ID: a loan holds a book ID and a member ID, looked up with `find_book()` and `find_member()`
- One struct that holds the whole library, passed by pointer
- Values worked out instead of stored: available copies and books on loan are counted from the loans
- Rules that keep linked data consistent, and checking every loan after loading a file
- Date arithmetic across months and years: `add_days()` and a day number since 1900-01-01
- Money in whole cents for the fines
- Two ways to delete: `memmove` to keep the members in order, and moving the last loan into the gap
- Forbidding the separator character instead of quoting it

## Tests

The files in `tests/` are typed input; the matching `.expected.txt` files are the exact output. Every test fixes the date with `--today`. `persist1` issues a book on 1 December and `persist2` returns it late on 20 December, then issues it again across the new year. They run with `ctest` from the [`mini-projects`](../) folder.

## Ideas to extend it

- Search books by title or author.
- Reservations: a fourth record type linking a member to a book that is out.
- Store the next ID numbers in the file, so IDs are never reused.
- A members' fine balance that must be paid before borrowing again.
