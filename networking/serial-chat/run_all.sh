#!/bin/sh
# Builds, tests and sanitizer-checks everything, then prints a Markdown table.
# Works on Ubuntu and on WSL. Needs cmake and gcc; clang and socat are used if present.
cd "$(dirname "$0")" || exit 1
os=$(. /etc/os-release 2>/dev/null; echo "${PRETTY_NAME:-unknown OS}")
grep -qi microsoft /proc/version 2>/dev/null && os="$os (WSL)"
tmp=$(mktemp -d)
rows=""
add() { rows="$rows| $1 | $2 |
"; }

command -v socat >/dev/null 2>&1 || add "two-terminal chat test" "skipped: socat not installed (sudo apt install socat)"
for cc in gcc clang; do
    command -v "$cc" >/dev/null 2>&1 || { add "$cc build and tests" "skipped: not installed"; continue; }
    for std in 11 17; do
        d="$tmp/$cc-$std"
        if cmake -S . -B "$d" -DCMAKE_C_COMPILER=$cc -DCMAKE_C_STANDARD=$std >/dev/null 2>&1 \
           && cmake --build "$d" >/dev/null 2>&1; then
            res=$(ctest --test-dir "$d" 2>&1 | grep "tests passed" | sed 's/^ *//')
            add "$cc, C$std, warnings as errors, ctest" "${res:-no ctest result}"
        else
            add "$cc, C$std, warnings as errors, ctest" "BUILD FAILED"
        fi
    done
done

f="-std=c11 -Wall -Wextra -pedantic -Iinclude -g -fsanitize=address,undefined -fno-sanitize-recover=all"
ok=1
gcc $f tests/test_frame.c src/frame.c -o "$tmp/t" 2>/dev/null && "$tmp/t" >/dev/null 2>&1 || ok=0
gcc $f src/frame_demo.c src/frame.c -o "$tmp/d1" 2>/dev/null && "$tmp/d1" >/dev/null 2>&1 || ok=0
gcc $f src/crc_experiment.c src/frame.c -o "$tmp/d2" 2>/dev/null && "$tmp/d2" 100 >/dev/null 2>&1 || ok=0
gcc $f src/pty_loopback.c src/serial_posix.c src/frame.c -lutil -o "$tmp/d3" 2>/dev/null && "$tmp/d3" >/dev/null 2>&1 || ok=0
[ $ok = 1 ] && add "AddressSanitizer + UBSan on the tests and demos" "clean" || add "AddressSanitizer + UBSan on the tests and demos" "REPORTED A PROBLEM"

printf '\nEnvironment: %s, %s, cmake %s\n\n| Check | Result |\n| --- | --- |\n%s' \
    "$os" "$(gcc --version | head -1)" "$(cmake --version | head -1 | cut -d' ' -f3)" "$rows"
rm -rf "$tmp"
