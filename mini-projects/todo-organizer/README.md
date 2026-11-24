# To-Do List Organizer in C (Dynamic Memory)

A to-do list kept in a plain text file. Each task has a text, a priority (high, medium or low), an optional due date and a done mark. The list shows tasks still to do first, then by priority, then by due date, and marks tasks that are past their due date `(overdue)` and tasks due today `(due today)`. You can add tasks, mark them done, delete them and remove all completed tasks at once, and every change is saved straight away. This is the first project in the series with dynamic memory: the list has no fixed size and grows with `realloc` as you add tasks, and each task's text takes exactly the memory it needs. The file has one task per line: the done flag, the priority, the due date (or `-`) and the text, which comes last so it may contain `|` itself:

```
0|1|2026-12-03|Write brief 13
0|3|-|Water | feed the plants
```

This project is part of [C Mini Projects for Beginners](https://www.mycplus.com/programming/c/mini-projects-in-c/#todo-organizer) on MYCPLUS.

## Build

With GCC or Clang (Linux, macOS, WSL):

```sh
gcc -std=c11 -Wall -Wextra -o todo_organizer todo_organizer.c
```

With Visual Studio, from a **Developer Command Prompt**:

```bat
cl /W4 todo_organizer.c
```

## Check for leaks yourself

```sh
gcc -std=c11 -g -fsanitize=address -o todo_organizer todo_organizer.c
```

Then use the program normally. When it exits, LeakSanitizer reports any memory that was allocated and never freed. This works on Linux; on macOS use the `leaks` tool, and with Visual Studio use the CRT debug heap. The repository's sanitizer test job builds exactly this way, so the tests also fail if the program ever leaks.

## Run

```sh
./todo_organizer                      # uses todo.txt and today's date
./todo_organizer --file work.txt      # a second list
./todo_organizer --today 2026-12-05   # pretend it is another day
```

On Windows, run `todo_organizer.exe` the same way.

## Sample session

```text
To-Do Organizer
===============
Today is 2026-12-01.
No file todo.txt yet: starting a new list.

1. List tasks
2. Add a task
3. Mark a task as done
4. Delete a task
5. Remove completed tasks
0. Quit
Choose an option (0-5): 2
Task: Pay rent
Priority (1 high, 2 medium, 3 low): 1
Due date (YYYY-MM-DD, or press Enter for none): 2026-12-02
Added task 1.

1. List tasks
2. Add a task
3. Mark a task as done
4. Delete a task
5. Remove completed tasks
0. Quit
Choose an option (0-5): 2
Task: Water | feed the plants
Priority (1 high, 2 medium, 3 low): 3
Due date (YYYY-MM-DD, or press Enter for none): 
Added task 2.

1. List tasks
2. Add a task
3. Mark a task as done
4. Delete a task
5. Remove completed tasks
0. Quit
Choose an option (0-5): 1

To-do list: 2 tasks, 0 done (room for 4)
  #  Done  Priority  Due         Task
  1  [ ]   high      2026-12-02  Pay rent
  2  [ ]   low       -           Water | feed the plants

1. List tasks
2. Add a task
3. Mark a task as done
4. Delete a task
5. Remove completed tasks
0. Quit
Choose an option (0-5): 0

Goodbye!
```

## Reused from earlier projects

| Function | From |
|---|---|
| `read_line`, `read_int` | [`unit_converter.c`](../unit-converter/) |
| `read_text` | [`grade_report.c`](../grade-report/) |
| `is_leap_year`, `is_valid_date` (with its `days_in_month` table inside it), `today_string` | [`personal_diary.c`](../personal-diary/) |

## What you practice

- A growing array: a pointer, a count and a capacity, grown with `realloc` from 4 by doubling
- Why the `realloc` result goes into a temporary pointer first, and why `realloc(NULL, size)` needs no special case
- One `malloc` per task text with your own `copy_string()`, since `strdup` was not standard C before C23
- Ownership: every `malloc` has exactly one `free`, and `list_free()` runs on every way out of the program
- Proving there are no leaks with AddressSanitizer and LeakSanitizer
- Reading a file line of any length by growing a buffer with `realloc`
- Sorting an array of pointers (a "view") with `qsort`, so the tasks keep their numbers
- Comparing `YYYY-MM-DD` dates with `strcmp`
- Removing many elements in one pass with a read index and a write index

## Tests

The files in `tests/` are typed input; the matching `.expected.txt` files are the exact output. Every test fixes the date with `--today`. `persist1` saves two tasks and `persist2` loads them two days later. They run with `ctest` from the [`mini-projects`](../) folder, and the sanitizer build of the same tests checks that nothing leaks.

## Ideas to extend it

- Edit a task's text: free the old text, then copy the new one.
- Categories or tags.
- Sort by due date only.
- Undo the last delete by keeping the removed task instead of freeing it at once.
