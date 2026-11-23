#!/bin/sh
# Builds and tests every mini project with GCC and Clang in C11 and C17,
# runs every session test under AddressSanitizer + UBSan, then prints a Markdown table.
# Works on Ubuntu and on WSL. Needs cmake and gcc; clang is used if present.
# Run it as: sh run_all.sh
cd "$(dirname "$0")" || exit 1
os=$(. /etc/os-release 2>/dev/null; echo "${PRETTY_NAME:-unknown OS}")
grep -qi microsoft /proc/version 2>/dev/null && os="$os (WSL)"
tmp=$(mktemp -d)
rows=""
add() { rows="$rows| $1 | $2 |
"; }

for cc in gcc clang; do
    command -v "$cc" >/dev/null 2>&1 || { add "$cc build and tests" "skipped: not installed"; continue; }
    for std in 11 17; do
        d="$tmp/$cc-$std"
        if cmake -S . -B "$d" -DCMAKE_C_COMPILER=$cc -DCMAKE_C_STANDARD=$std >/dev/null 2>&1 \
           && cmake --build "$d" >/dev/null 2>&1; then
            res=$(ctest --test-dir "$d" 2>&1 | grep "tests passed" | sed 's/^ *//')
            add "$cc, C$std, -Wall -Wextra -pedantic -Werror, ctest" "${res:-no ctest result}"
        else
            add "$cc, C$std, -Wall -Wextra -pedantic -Werror, ctest" "BUILD FAILED"
        fi
    done
done

d="$tmp/asan"
if cmake -S . -B "$d" -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug \
         "-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -g" >/dev/null 2>&1 \
   && cmake --build "$d" >/dev/null 2>&1; then
    res=$(ctest --test-dir "$d" 2>&1 | grep "tests passed" | sed 's/^ *//')
    add "AddressSanitizer + UBSan (gcc, Debug), ctest" "${res:-REPORTED A PROBLEM}"
else
    add "AddressSanitizer + UBSan (gcc, Debug), ctest" "BUILD FAILED"
fi

printf '\nEnvironment: %s, %s, cmake %s\n\n| Check | Result |\n| --- | --- |\n%s' \
    "$os" "$(gcc --version | head -1)" "$(cmake --version | head -1 | cut -d' ' -f3)" "$rows"
rm -rf "$tmp"
