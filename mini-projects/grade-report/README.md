# Student Grade Report in C

A teacher's report card for one class. The program asks for the subjects (1 to 6) and the students (1 to 30), then each student's name and a mark from 0 to 100 in every subject. It prints a report card with every student's marks, total, average and letter grade, then each subject's average, highest and lowest mark, the class average, the top student and how many students got each grade. There is no menu: it reads the class, prints the report once and exits.

This project is part of [C Mini Projects for Beginners](https://www.mycplus.com/programming/c/mini-projects-in-c/#grade-report) on MYCPLUS.

## Build

With GCC or Clang (Linux, macOS, WSL):

```sh
gcc -std=c11 -Wall -Wextra -o grade_report grade_report.c
```

With Visual Studio, from a **Developer Command Prompt**:

```bat
cl /W4 grade_report.c
```

## Run

```sh
./grade_report                       # type the class in
./grade_report < tests/session.txt   # see the full report without typing
```

On Windows, run `grade_report.exe` the same way.

## Sample report card

The report printed for `tests/session.txt` (3 subjects, 5 students):

```text
REPORT CARD
===========
Name                    Math Physics    Chem   Total  Average  Grade
Aisha                     92      88      95     275    91.67  A
Ben                       92      85      80     257    85.67  B
Chen                      60      59      61     180    60.00  D
Dana                      90      90      90     270    90.00  A
Eve                       50      45      62     157    52.33  F
--------------------------------------------------------------------
Average                76.80   73.40   77.60
Highest                   92      90      95
Lowest                    50      45      61

Class average: 75.93
Top student:   Aisha (total 275, average 91.67)
Grade counts:  A: 2  B: 1  C: 0  D: 1  F: 1
```

## What you practice

- One-dimensional arrays: an array of totals and a letter grade for each student
- Two-dimensional arrays: `int marks[MAX_STUDENTS][MAX_SUBJECTS]`, rows for students and columns for subjects, walked with nested loops
- Names as a 2D `char` array (`char names[MAX_STUDENTS][NAME_SIZE]`), compared with Hangman's array of pointers
- Accumulators: a running sum, and a running highest and lowest that start from the first mark, not from 0 or 100
- The integer division trap: `275 / 3` is `91`, so averages use `(double)total / count`, which gives 91.67
- Passing arrays to functions: the size goes in a separate parameter, and a 2D parameter needs its column size (`int marks[][MAX_SUBJECTS]`)
- A lookup table for grades: cutoffs `90, 80, 70, 60` and letters `"ABCD"`, with anything below 60 an `F`
- Formatted tables with `printf` widths: `%-20s`, `%8d`, `%9.2f`

## Tests

`tests/session.txt` and `tests/eof.txt` are typed input; the matching `.expected.txt` files are the exact output. They run with `ctest` from the [`mini-projects`](../) folder.

## Ideas to extend it

- Save the report to a text file.
- Sort the students by total before printing.
- Show who scored the highest mark in each subject, listing every student on a tie.
- Read the class from a CSV file instead of the keyboard.
