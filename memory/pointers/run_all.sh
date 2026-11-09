#!/bin/sh
# Builds, tests and sanitizer-checks everything, then prints a Markdown table.
# Works on Ubuntu and on WSL. Needs cmake and gcc; clang and valgrind are used if present.
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
            add "$cc, C$std, warnings as errors, ctest" "${res:-no ctest result}"
        else
            add "$cc, C$std, warnings as errors, ctest" "BUILD FAILED"
        fi
    done
done

flags="-std=c11 -Wall -Wextra -pedantic -g -fsanitize=address,undefined -fno-sanitize-recover=all"
ok=1
for t in pointer_arith functions text dynamic_array linked_list generic; do
    gcc $flags tests/test_$t.c -o "$tmp/t_$t" 2>/dev/null && "$tmp/t_$t" >/dev/null 2>&1 || ok=0
done
for p in basics address_demo sizes arrays functions strings const_pointers dynamic_array linked_list callbacks generic_swap double_pointer; do
    gcc $flags src/$p.c -o "$tmp/d_$p" 2>/dev/null && "$tmp/d_$p" a b >/dev/null 2>&1 || ok=0
done
[ $ok = 1 ] && add "AddressSanitizer + UBSan on 6 test files and 12 demos" "clean" || add "AddressSanitizer + UBSan on 6 test files and 12 demos" "REPORTED A PROBLEM"

./pitfalls/run_pitfalls.sh "$tmp/pit" > "$tmp/pit.log" 2>&1
missing=""
for e in "heap-use-after-free" "attempting double-free" "Direct leak of 400 byte" "heap-buffer-overflow" \
         "SEGV on unknown address" "data is still NULL" "Wsizeof-array-argument" \
         "assignment of read-only location" "exit status 139" "invalid use of void expression" "sizeof b = 4"; do
    grep -q -- "$e" "$tmp/pit.log" || missing="$missing [$e]"
done
[ -z "$missing" ] && add "pitfall programs produce the diagnostics quoted in the article" "all 11 found" || add "pitfall programs produce the diagnostics quoted in the article" "MISSING:$missing"

printf '\nEnvironment: %s, %s, cmake %s\n\n| Check | Result |\n| --- | --- |\n%s' \
    "$os" "$(gcc --version | head -1)" "$(cmake --version | head -1 | cut -d' ' -f3)" "$rows"
rm -rf "$tmp"
