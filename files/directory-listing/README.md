# Directory Listing in C

[![directory-listing](https://github.com/mycplus/c-examples/actions/workflows/directory-listing.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/directory-listing.yml)

Companion code for [List Files in a Directory in C: dir, ls and tree for Linux and Windows](https://www.mycplus.com/programming/c/list-files-in-directory/) on MYCPLUS. One small layer hides the difference between POSIX (`opendir`, `readdir`, `fstatat`) and Windows (`FindFirstFileW`), and three programs are built on it.

| File | What it is |
| --- | --- |
| `src/fs.h`, `src/fs_common.c` | The portable interface: open, read and close a directory, read it sorted, join paths safely, format times |
| `src/fs_posix.c` | Linux and macOS: `opendir`/`readdir`, with `fstatat(..., AT_SYMLINK_NOFOLLOW)` for each entry |
| `src/fs_win32.c` | Windows: `FindFirstFileW`/`FindNextFileW`, with names converted to UTF-8 |
| `src/walk.h`, `src/walk.c` | Reads a whole tree into memory without following symbolic links, and adds up directory sizes |
| `src/dirlist.c` | Lists one directory, like `ls -l` or `dir`: `dirlist [-a] [-d \| -f] [-u] [-s] [DIRECTORY]` |
| `src/tree.c` | Prints a directory's structure as a tree: `tree [-a] [-s] [-L LEVELS] [DIRECTORY]` |
| `src/bigfiles.c` | Finds the largest files below a directory: `bigfiles [-a] DIRECTORY [COUNT]` |
| `src/make_fixture.c` | Builds the small, fixed tree the tests and the article list |
| `pitfalls/stat_wrong_path.c` | `stat()` on the bare name `readdir()` returns (POSIX) |
| `pitfalls/symlink_loop.c` | A walker that follows a link pointing at its own directory (POSIX) |
| `pitfalls/path_buffer.c` | Building a path with `strcpy` and `strcat` in a fixed buffer (`checked` / `unchecked`) |
| `tests/` | Unit tests, expected outputs and mutation tests |
| `run_all.sh`, `run_all.ps1` | One-command verification for Linux and for Windows (MSVC); each prints a Markdown report |

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/tree -s ..
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 with `-Wall -Wextra -pedantic -Werror`, and with MSVC under `/W4 /WX`. `pitfalls/path_buffer.c` is the one exception: its overflow is the lesson, and GCC warns about it.
- The unit tests check path joining, sorting, sizes, times, hidden entries, a UTF-8 file name, error messages, empty directories and, on Linux and macOS, that symbolic links are reported and never followed.
- `dirlist`, `tree`, `bigfiles` and the pitfalls print exactly the output shown in the article. The same expected output is used on Linux and Windows.
- `tests/mutations.py` puts 13 mistakes into the library and confirms the tests catch each one.
