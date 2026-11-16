#!/bin/sh
# Builds, tests and sanitizer-checks everything, then prints a Markdown table.
# Works on Ubuntu and on WSL. Needs cmake, gcc and bash; clang is used if present.
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

f="-std=c11 -Wall -Wextra -pedantic -Iinclude -g -fsanitize=address,undefined -fno-sanitize-recover=all"
ok=1
mkdir -p "$tmp/san"
for p in udp-sender udp-receiver boundaries connected loss_test; do
    gcc $f src/$p.c src/udp.c -o "$tmp/san/$p" 2>/dev/null || ok=0
done
"$tmp/san/boundaries" >/dev/null 2>&1 || ok=0
"$tmp/san/connected" >/dev/null 2>&1 || ok=0
"$tmp/san/loss_test" 3000 4096 >/dev/null 2>&1 || ok=0
bash tests/sender_receiver_test.sh "$tmp/san" >/dev/null 2>&1 || ok=0
[ $ok = 1 ] && add "AddressSanitizer + UBSan on all programs and the sender/receiver test" "clean" || add "AddressSanitizer + UBSan on all programs and the sender/receiver test" "REPORTED A PROBLEM"

loss=$("$tmp/san/loss_test" 10000 4096 2>/dev/null | tail -1)
add "loss_test 10000 datagrams, 4096-byte receive buffer" "${loss:-not run}"

printf '\nEnvironment: %s, %s, cmake %s\n\n| Check | Result |\n| --- | --- |\n%s' \
    "$os" "$(gcc --version | head -1)" "$(cmake --version | head -1 | cut -d' ' -f3)" "$rows"
rm -rf "$tmp"
