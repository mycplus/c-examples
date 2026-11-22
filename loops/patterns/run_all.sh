#!/bin/sh
# Builds and tests everything with GCC and Clang (C11 and C17), runs the
# sanitizers, Valgrind and the mutation tests, and prints a Markdown report.
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

d="$tmp/asan"
if cmake -S . -B "$d" -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug \
         "-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -g" >/dev/null 2>&1 \
   && cmake --build "$d" >/dev/null 2>&1; then
    res=$(ctest --test-dir "$d" 2>&1 | grep "tests passed" | sed 's/^ *//')
    add "AddressSanitizer + UBSan (gcc), ctest" "${res:-REPORTED A PROBLEM}"
fi

if command -v valgrind >/dev/null 2>&1 && [ -x "$tmp/gcc-11/test_patterns" ]; then
    ok=1
    valgrind -q --error-exitcode=9 --leak-check=full "$tmp/gcc-11/test_patterns" >/dev/null 2>&1 || ok=0
    valgrind -q --error-exitcode=9 "$tmp/gcc-11/barchart" data/downloads.txt >/dev/null 2>&1 || ok=0
    valgrind -q --error-exitcode=9 "$tmp/gcc-11/pyramid" < tests/input-5.txt >/dev/null 2>&1 || ok=0
    [ $ok = 1 ] && add "Valgrind memcheck: unit tests, barchart, pyramid" "no errors, no leaks" \
                || add "Valgrind memcheck: unit tests, barchart, pyramid" "REPORTED A PROBLEM"
fi

if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
    ok=1
    for p in "src/patterns.c src/patterns_cli.c" src/pyramid.c src/shapes.c src/barchart.c "src/patterns.c tests/test_patterns.c"; do
        # shellcheck disable=SC2086
        x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -pedantic -Werror -Isrc $p -o "$tmp/x.exe" 2>>"$tmp/mingw.log" || ok=0
    done
    [ $ok = 1 ] && add "Cross-compiled for Windows with MinGW-w64, warnings as errors" "passed" \
                || add "Cross-compiled for Windows with MinGW-w64, warnings as errors" "FAILED"
fi

if command -v python3 >/dev/null 2>&1; then
    python3 tests/mutations.py gcc >"$tmp/mut.txt" 2>&1
    add "Mutation tests (10 mistakes in patterns.c)" "$(tail -1 "$tmp/mut.txt")"
fi

printf '\nEnvironment: %s, %s, %s, cmake %s\n\n| Check | Result |\n| --- | --- |\n%s' \
    "$os" "$(gcc --version | head -1)" "$(clang --version 2>/dev/null | head -1)" \
    "$(cmake --version | head -1 | cut -d' ' -f3)" "$rows"
if [ -s "$tmp/mut.txt" ]; then printf '\n```\n'; cat "$tmp/mut.txt"; printf '```\n'; fi
rm -rf "$tmp"
