#!/bin/sh
# Builds and tests everything with GCC and Clang (C11 and C17), runs the
# sanitizers, Valgrind and the mutation tests, and prints a Markdown report.
# Needs cmake, gcc and python3; clang and valgrind are used if present.
#   sh run_all.sh > results-linux.md
cd "$(dirname "$0")" || exit 1
os=$(. /etc/os-release 2>/dev/null; echo "${PRETTY_NAME:-unknown OS}")
tmp=$(mktemp -d)
rows=""
add() { rows="$rows| $1 | $2 |
"; }

for cc in gcc clang; do
    command -v "$cc" >/dev/null 2>&1 || { add "$cc build and tests" "skipped: not installed"; continue; }
    for std in 11 17; do
        d="$tmp/$cc-$std"
        if cmake -S . -B "$d" -DCMAKE_C_COMPILER=$cc -DCMAKE_C_STANDARD=$std >/dev/null 2>&1 \
           && cmake --build "$d" >"$tmp/build.log" 2>&1; then
            res=$(ctest --test-dir "$d" 2>&1 | grep "tests passed" | sed 's/^ *//')
            add "$cc, C$std, warnings as errors, ctest" "${res:-no ctest result}"
        else
            cp "$tmp/build.log" "build-$cc-c$std.log"
            add "$cc, C$std, warnings as errors, ctest" "BUILD FAILED (see build-$cc-c$std.log)"
        fi
    done
done

san="-fsanitize=address,undefined -fno-sanitize-recover=all -g"
d="$tmp/asan"
if cmake -S . -B "$d" -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_C_FLAGS=$san" >/dev/null 2>&1 \
   && cmake --build "$d" >/dev/null 2>&1; then
    res=$(ctest --test-dir "$d" 2>&1 | grep "tests passed" | sed 's/^ *//')
    add "AddressSanitizer + UBSan (gcc), ctest" "${res:-REPORTED A PROBLEM}"
    if "$d/pitfall_path_buffer" unchecked >/dev/null 2>"$tmp/pb.txt"; then
        add "Unchecked strcat path, under ASan" "NOT DETECTED"
    else
        add "Unchecked strcat path, under ASan" "$(grep -oE "AddressSanitizer: [a-z-]+" "$tmp/pb.txt" | head -1)"
    fi
else
    add "AddressSanitizer + UBSan (gcc), ctest" "BUILD FAILED"
fi

if command -v valgrind >/dev/null 2>&1 && [ -x "$tmp/gcc-11/test_fs" ]; then
    ok=1
    (cd "$tmp/gcc-11" && valgrind -q --error-exitcode=9 --leak-check=full ./test_fs fixture vg-scratch >/dev/null 2>&1) || ok=0
    for p in "dirlist -a -u fixture" "tree -a -s fixture" "bigfiles fixture 3"; do
        # shellcheck disable=SC2086
        (cd "$tmp/gcc-11" && valgrind -q --error-exitcode=9 --leak-check=full ./$p >/dev/null 2>&1) || ok=0
    done
    [ $ok = 1 ] && add "Valgrind memcheck: tests, dirlist, tree, bigfiles" "no errors, no leaks" \
                || add "Valgrind memcheck: tests, dirlist, tree, bigfiles" "REPORTED A PROBLEM"
fi

if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
    ok=1
    for p in dirlist tree bigfiles make_fixture; do
        x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -pedantic -Werror -Isrc src/fs_common.c src/walk.c \
            src/fs_win32.c "src/$p.c" -o "$tmp/$p.exe" 2>>"$tmp/mingw.log" || ok=0
    done
    [ $ok = 1 ] && add "Windows code cross-compiled with MinGW-w64, warnings as errors" "passed" \
                || add "Windows code cross-compiled with MinGW-w64, warnings as errors" "FAILED"
fi

if command -v python3 >/dev/null 2>&1; then
    python3 tests/mutations.py gcc >"$tmp/mut.txt" 2>&1
    add "Mutation tests (13 mistakes in the library)" "$(tail -1 "$tmp/mut.txt")"
fi

printf '\nEnvironment: %s, %s, %s, cmake %s\n\n| Check | Result |\n| --- | --- |\n%s' \
    "$os" "$(gcc --version | head -1)" "$(clang --version 2>/dev/null | head -1)" \
    "$(cmake --version | head -1 | cut -d' ' -f3)" "$rows"
if [ -s "$tmp/mut.txt" ]; then printf '\n```\n'; cat "$tmp/mut.txt"; printf '```\n'; fi
rm -rf "$tmp"
