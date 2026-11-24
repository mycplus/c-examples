# Personal Diary in C

A diary kept in a plain text file. Write an entry and the program stamps it with today's date; read every entry, numbered; or search the entries for a word or phrase, ignoring upper and lower case. The file *is* the diary: close the program, run it again tomorrow, and the old entries are still there. Each line of the file is one entry, so you can also open it in any text editor:

```
2026-12-01 | Started the C mini projects.
2026-12-01 | Finished the Unit Converter.
2026-12-02 | Hangman was more fun than expected.
```

This project is part of [C Mini Projects for Beginners](https://www.mycplus.com/programming/c/mini-projects-in-c/#personal-diary) on MYCPLUS.

## Build

With GCC or Clang (Linux, macOS, WSL):

```sh
gcc -std=c11 -Wall -Wextra -o personal_diary personal_diary.c
```

With Visual Studio, from a **Developer Command Prompt**:

```bat
cl /W4 personal_diary.c
```

## Run

```sh
./personal_diary                     # uses diary.txt and today's date
./personal_diary --file work.txt     # keep a second diary
./personal_diary --today 2026-12-01  # pretend it is another day
```

On Windows, run `personal_diary.exe` the same way. The options can come in any order.

## Sample session

```text
Personal Diary
==============
Diary file: diary.txt
Today is 2026-12-01.

1. Write an entry
2. Read all entries
3. Search
0. Quit
Choose an option (0-3): 1
Entry for 2026-12-01: Started the C mini projects.
Saved.

1. Write an entry
2. Read all entries
3. Search
0. Quit
Choose an option (0-3): 1
Entry for 2026-12-01: Finished the Unit Converter.
Saved.

1. Write an entry
2. Read all entries
3. Search
0. Quit
Choose an option (0-3): 3
Search for: unit

1 entry contains "unit":
  2. 2026-12-01  Finished the Unit Converter.

1. Write an entry
2. Read all entries
3. Search
0. Quit
Choose an option (0-3): 0

Goodbye!
```

## What you practice

- File modes: `"a"` appends and creates the file if needed, `"r"` reads, and `"w"` would erase the whole diary every time
- `fopen` can fail: a missing file (`errno == ENOENT`) just means an empty diary, and any other failure is reported with `strerror(errno)`
- Reading a text file line by line with `fgets`, removing the newline, and splitting each line at the first `" | "` with `strstr`
- Dates with `<time.h>`: `time(NULL)`, `localtime()` and `strftime()`
- Checking a date by hand, including the leap-year rule (2024 and 2000 are leap years; 1900 and 2026 are not)
- Case-insensitive search without non-standard functions such as `strcasestr`, using `tolower()` in two nested loops
- Command-line options with values, `--file NAME` and `--today YYYY-MM-DD`, in any order

## Tests

The files in `tests/` are typed input; the matching `.expected.txt` files are the exact output. Each test runs in its own build folder and deletes its diary file first. `persist1` and `persist2` run one after the other and prove that the diary survives between runs. They run with `ctest` from the [`mini-projects`](../) folder.

## Ideas to extend it

- Entries that span several lines.
- List the entries for one date or one month.
- Delete or edit an entry: read everything, change one line, write a new file, then rename it over the old one.
- A simple password that scrambles the text.
