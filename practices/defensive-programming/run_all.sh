#!/bin/sh
# The Linux checks behind the article's "What I Verified" table:
#   1. GCC and Clang, C11 and C17, warnings as errors, CTest.
#   2. AddressSanitizer and UBSan: CTest, then three programs with deliberate
#      memory bugs that the sanitizers must stop.
#   3. Valgrind, and GCC's -Wall and -fanalyzer on the pitfalls and the library.
#   4. Four deliberate mistakes in the library that the tests must catch.
set -u
cd "$(dirname "$0")"
status=0
tmp=$(mktemp -d)
report() { printf '%-62s %s\n' "$1" "$2"; }

build() { # dir, extra cmake arguments...
    d=$1; shift
    cmake -S . -B "$d" "$@" >"$d.log" 2>&1 && cmake --build "$d" -j 4 >>"$d.log" 2>&1
}

for cc in gcc clang; do
    for std in 11 17; do
        if build "$tmp/$cc-$std" -DCMAKE_C_COMPILER=$cc -DCMAKE_C_STANDARD=$std &&
           ctest --test-dir "$tmp/$cc-$std" >"$tmp/ctest.log" 2>&1; then
            report "$cc C$std build and CTest" "PASS ($(grep -o '[0-9]* of [0-9]* checks passed' -r "$tmp/$cc-$std/Testing" 2>/dev/null | head -1 | sed 's/.*://'))"
        else
            report "$cc C$std build and CTest" FAIL; status=1
        fi
    done
done
"$tmp/gcc-11/test_defensive" data "$tmp" | tail -1 | sed 's/^/unit tests: /'

san="-fsanitize=address,undefined -fno-sanitize-recover=all -g"
if build "$tmp/asan" -DCMAKE_C_COMPILER=gcc "-DCMAKE_C_FLAGS=$san" &&
   ctest --test-dir "$tmp/asan" >/dev/null 2>&1; then
    report "gcc ASan and UBSan, CTest" PASS
else
    report "gcc ASan and UBSan, CTest" FAIL; status=1
fi
detect() { # label, program, pattern
    if "$tmp/asan/$2" >/dev/null 2>"$tmp/san.txt"; then
        report "$1" "NOT DETECTED"; status=1
    elif msg=$(grep -oE "$3" "$tmp/san.txt" | head -1); then
        report "$1" "$msg"
    else
        report "$1" "stopped, but not by the sanitizer"; status=1
    fi
}
detect "Sanitizer: off-by-one malloc" pitfall_off_by_one "heap-buffer-overflow"
detect "Sanitizer: read after free" pitfall_use_after_free "heap-use-after-free"
detect "Sanitizer: INT_MAX + 1" pitfall_signed_overflow "signed integer overflow"
"$tmp/gcc-11/pitfall_off_by_one" >"$tmp/plain.txt" 2>&1
report "Same off-by-one without a sanitizer" "exit status $?, printed: $(head -c 40 "$tmp/plain.txt")"

if command -v valgrind >/dev/null 2>&1; then
    if valgrind -q --error-exitcode=9 --leak-check=full "$tmp/gcc-11/test_defensive" data "$tmp" >/dev/null 2>&1 &&
       { valgrind -q --error-exitcode=9 --leak-check=full "$tmp/gcc-11/people" data/people.txt >/dev/null 2>&1; [ $? -ne 9 ]; }; then
        report "Valgrind memcheck: unit tests and people" "no errors, no leaks"
    else
        report "Valgrind memcheck: unit tests and people" "REPORTED A PROBLEM"; status=1
    fi
fi

warn() { # label, flags, file, pattern
    if gcc -std=c11 $2 -c "$3" -o /dev/null >"$tmp/w.txt" 2>&1 && ! grep -q warning "$tmp/w.txt"; then
        out=""
    else
        out=$(grep -oE "$4" "$tmp/w.txt" | head -1)
    fi
    report "$1" "${out:-no warning}"
}
warn "gcc -Wall: read after free" "-Wall" pitfalls/use_after_free.c "\[-Wuse-after-free[^]]*\]"
warn "gcc -fanalyzer: leak on the error path" "-fanalyzer" pitfalls/leak_on_error.c "\[-Wanalyzer-[a-z-]*\]"
warn "gcc -fanalyzer: defensive.c" "-fanalyzer -Wall -Wextra" src/defensive.c "\[-W[a-z-]*\]"
warn "gcc -fanalyzer: people.c" "-fanalyzer -Wall -Wextra -Isrc" src/people.c "\[-W[a-z-]*\]"

mutant() { # label, sed expression on src/defensive.c
    rm -rf "$tmp/m" && mkdir "$tmp/m" && cp -r CMakeLists.txt src tests data pitfalls compile_fail "$tmp/m/"
    sed -i "$2" "$tmp/m/src/defensive.c"
    if cmp -s src/defensive.c "$tmp/m/src/defensive.c"; then report "$1" "NOT APPLIED"; status=1; return; fi
    if (cd "$tmp/m" && build "$tmp/mb" -DCMAKE_C_COMPILER=gcc -S "$tmp/m" && ctest --test-dir "$tmp/mb" >/dev/null 2>&1); then
        report "$1" MISSED; status=1
    else
        report "$1" CAUGHT
    fi
    rm -rf "$tmp/mb"
}
mutant "Mutant 1: text after a number accepted" 's/if (\*end != .\\0.) {/if (0) {/'
mutant "Mutant 2: a line that fills the buffer called too long" "s/if (c != '\\\\n' \&\& c != EOF) {/if (1) {/"
mutant "Mutant 3: no overflow check before multiplying" 's/if (b != 0 \&\& a > SIZE_MAX \/ b) {/if (0) {/'
mutant "Mutant 4: an existing file deleted after a failed copy" 's/if (status != DP_OK \&\& created) {/if (status != DP_OK) {/'

rm -rf "$tmp"
exit $status
