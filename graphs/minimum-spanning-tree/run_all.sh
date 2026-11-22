#!/bin/sh
# The Linux (Ubuntu or WSL) checks behind the article's "What I Verified" table:
#   1. GCC and Clang, C11 and C17 (C++17 alongside), warnings as errors, CTest.
#   2. AddressSanitizer and UBSan on every test; UBSan must stop the
#      subtracting comparator.
#   3. Valgrind on the C tests and the C++ self-test.
#   4. Eight deliberate mistakes in the library that the tests must catch.
# Builds go to a temporary directory, so nothing is left in the repository.
# Run from anywhere:  sh run_all.sh | tee results-wsl.md
set -u
cd "$(dirname "$0")"
status=0
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
report() { printf '%-56s %s\n' "$1" "$2"; }
build() { # dir, extra cmake arguments...
    d=$1; shift
    cmake -S . -B "$d" "$@" >"$d.log" 2>&1 && cmake --build "$d" -j 4 >>"$d.log" 2>&1
}

echo "Environment: $(uname -sr), $(grep -s PRETTY_NAME /etc/os-release | cut -d'"' -f2)"
grep -qi microsoft /proc/version 2>/dev/null && echo "Running under WSL"
gcc --version 2>/dev/null | head -1
clang --version 2>/dev/null | head -1
echo

for cc in gcc clang; do
    cxx=g++; [ "$cc" = clang ] && cxx=clang++
    for std in 11 17; do
        if build "$tmp/$cc-$std" -DCMAKE_C_COMPILER=$cc -DCMAKE_CXX_COMPILER=$cxx -DCMAKE_C_STANDARD=$std &&
           ctest --test-dir "$tmp/$cc-$std" >"$tmp/ctest.log" 2>&1; then
            report "$cc C$std + C++17, build and CTest" "PASS ($(grep -o '[0-9]* tests failed out of [0-9]*' "$tmp/ctest.log" | sed 's/.* out of /all /') tests)"
        else
            report "$cc C$std + C++17, build and CTest" "FAIL (see $tmp/$cc-$std.log)"; status=1
        fi
    done
done

san="-fsanitize=address,undefined -fno-sanitize-recover=all -g"
if build "$tmp/asan" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ "-DCMAKE_C_FLAGS=$san" "-DCMAKE_CXX_FLAGS=$san" &&
   ctest --test-dir "$tmp/asan" >/dev/null 2>&1; then
    report "gcc ASan and UBSan, CTest" PASS
else
    report "gcc ASan and UBSan, CTest" FAIL; status=1
fi
if "$tmp/asan/pitfall_subtract_compare" >/dev/null 2>"$tmp/ub.txt"; then
    report "UBSan stops the subtracting comparator" "NOT DETECTED"; status=1
else
    report "UBSan stops the subtracting comparator" "$(grep -o 'signed integer overflow' "$tmp/ub.txt" | head -1)"
fi

if command -v valgrind >/dev/null 2>&1; then
    if valgrind -q --error-exitcode=9 --leak-check=full "$tmp/gcc-11/test_mst" >/dev/null 2>&1 &&
       valgrind -q --error-exitcode=9 --leak-check=full "$tmp/gcc-11/mst_cpp" --test >/dev/null 2>&1; then
        report "Valgrind: C tests and C++ self-test" "no errors, no leaks"
    else
        report "Valgrind: C tests and C++ self-test" "REPORTED A PROBLEM"; status=1
    fi
else
    report "Valgrind: C tests and C++ self-test" "SKIP (sudo apt install valgrind)"
fi

if python3 tests/mutations.py gcc >"$tmp/mut.txt" 2>&1; then
    report "Mutation tests (8 mistakes, 1 harmless change)" PASS
else
    report "Mutation tests (8 mistakes, 1 harmless change)" FAIL; status=1; cat "$tmp/mut.txt"
fi
exit $status
