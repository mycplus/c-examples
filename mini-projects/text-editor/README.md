# Text Editor in C (Line Editor)

A line editor in the tradition of Unix `ed`. Instead of moving a cursor around a screen, you work on the file with short commands typed at the `edit> ` prompt: `p` prints lines, `a` adds lines after any line, `c` changes a line, `d` deletes one line or a range, `f` finds lines containing a word, `r` replaces text in every line, `w` writes the file (or saves it under a new name) and `q` quits. Each line has a number, and commands use those numbers. There are no cursor keys and no screen control, so the program is standard C and runs anywhere. Lines can be any length, both in the file and when typed, and indentation is kept exactly. The prompt becomes `edit*> ` while there are unsaved changes, and quitting asks whether to save them.

This project is part of [C Mini Projects for Beginners](https://www.mycplus.com/programming/c/mini-projects-in-c/#text-editor) on MYCPLUS.

## Build

With GCC or Clang (Linux, macOS, WSL):

```sh
gcc -std=c11 -Wall -Wextra -o text_editor text_editor.c
```

With Visual Studio, from a **Developer Command Prompt**:

```bat
cl /W4 text_editor.c
```

## Run

```sh
./text_editor notes.txt      # Windows: text_editor.exe notes.txt
```

If `notes.txt` does not exist yet, the editor starts a new, empty file with that name.

## Commands

```text
Commands (N and M are line numbers):
  p [N [M]]   print all lines, line N, or lines N to M
  a [N]       add lines after line N (0 = at the top; default: at the end)
  c N         change line N
  d N [M]     delete line N, or lines N to M
  f WORD      find lines containing WORD (ignoring case)
  r           replace text in every line (exact match)
  w [NAME]    write the file, or save it as NAME
  q           quit
  h           show this help
```

When adding lines, type a line with just `.` to stop. Saving writes a newline after every line, so a file whose last line had no newline gains one.

## Sample session

```text
Text Editor
===========
Editing notes.txt (new file).
Type h for help.
edit> a
Adding after line 0. Type a line with just . to stop.
one fish
two fish
red fish blue fish
.
Added 3 lines.
edit*> r
Find: fish
Replace with: cat
Replaced 4 occurrences in 3 lines.
edit*> p
   1  one cat
   2  two cat
   3  red cat blue cat
edit*> w
Wrote 3 lines to notes.txt.
edit> q

Goodbye!
```

## Reused from earlier projects

| Function | From |
|---|---|
| `read_line` | [`unit_converter.c`](../unit-converter/) |
| `contains_ignore_case` | [`personal_diary.c`](../personal-diary/) |
| `ask_yes_no` | [`address_book.c`](../address-book/) |
| `copy_string`, `read_file_line` (and the `FIRST_LINE` constant it uses) | [`todo_organizer.c`](../todo-organizer/) |

## What you practice

- The text as an array of pointers to strings (`char **lines`), growing with `realloc`
- Inserting and deleting in the middle by moving pointers, not text, with `memmove`
- Reading typed text of any length with the same function that reads the file: `stdin` is a `FILE *` too
- Building a new string of exactly the right size: count the matches, `malloc` once, then copy
- Parsing a command: a letter, then up to two line numbers read with `strtol` and checked
- An "unsaved changes" flag shown in the prompt itself
- The safe save, and "save as" with `w NAME`
- Ownership: every line is freed exactly once, checked by LeakSanitizer in the tests
- Reusing tested functions from earlier projects

## Tests

The files in `tests/` are typed input; the matching `.expected.txt` files are the exact output. Each test runs in its own build folder and deletes its file first. `persist1` writes a file and `persist2` opens it again, adds a line at the top and replaces text. They run with `ctest` from the [`mini-projects`](../) folder, and the sanitizer build checks that nothing leaks.

## Ideas to extend it

- Undo for the last change: keep the removed lines instead of freeing them at once.
- An `m N M` command to move a line.
- Search and replace on a range of lines only.
- A full-screen editor with `ncurses` (Linux and macOS) or the Windows Console API. That is no longer standard C, which is why this project stays line-based.
