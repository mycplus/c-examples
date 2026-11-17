#!/bin/sh
# Builds, tests and sanitizer-checks everything, runs the mutation tests and the
# timing benchmark, then prints a Markdown report. Works on Ubuntu and on WSL.
# Needs cmake, gcc, g++ and python3; clang and valgrind are used if present.
#   sh run_all.sh > results-wsl.md
cd "$(dirname "$0")" || exit 1
os=$(. /etc/os-release 2>/dev/null; echo "${PRETTY_NAME:-unknown OS}")
grep -qi microsoft /proc/version 2>/dev/null && os="$os (WSL)"
tmp=$(mktemp -d)
rows=""
add() { rows="$rows| $1 | $2 |
"; }

for cc in gcc clang; do
    cxx=g++; [ "$cc" = clang ] && cxx=clang++
    command -v "$cc" >/dev/null 2>&1 && command -v "$cxx" >/dev/null 2>&1 ||
        { add "$cc build and tests" "skipped: not installed"; continue; }
    for std in 11 17; do
        d="$tmp/$cc-$std"
        if cmake -S . -B "$d" -DCMAKE_C_COMPILER=$cc -DCMAKE_CXX_COMPILER=$cxx -DCMAKE_C_STANDARD=$std >/dev/null 2>&1 \
           && cmake --build "$d" >"$tmp/build.log" 2>&1; then
            res=$(ctest --test-dir "$d" 2>&1 | grep "tests passed" | sed 's/^ *//')
            add "$cc, C$std + C++17, warnings as errors, ctest" "${res:-no ctest result}"
        else
            cp "$tmp/build.log" "build-$cc-c$std.log"
            add "$cc, C$std + C++17, warnings as errors, ctest" "BUILD FAILED (see build-$cc-c$std.log)"
        fi
    done
done

san="-fsanitize=address,undefined -fno-sanitize-recover=all -g"
d="$tmp/asan"
if cmake -S . -B "$d" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Debug \
         "-DCMAKE_C_FLAGS=$san" "-DCMAKE_CXX_FLAGS=$san" >/dev/null 2>&1 && cmake --build "$d" >/dev/null 2>&1; then
    res=$(ctest --test-dir "$d" 2>&1 | grep "tests passed" | sed 's/^ *//')
    add "AddressSanitizer + UBSan (gcc), ctest" "${res:-REPORTED A PROBLEM}"
else
    add "AddressSanitizer + UBSan (gcc), ctest" "BUILD FAILED"
fi

if command -v valgrind >/dev/null 2>&1 && [ -x "$tmp/gcc-11/route" ]; then
    ok=1
    valgrind -q --error-exitcode=9 --leak-check=full "$tmp/gcc-11/test_shortest_path" data/lakeland.txt >/dev/null 2>&1 || ok=0
    for args in "Ashford Larkspur" "Larkspur Ashford floyd" "--table"; do
        # shellcheck disable=SC2086
        valgrind -q --error-exitcode=9 --leak-check=full "$tmp/gcc-11/route" data/lakeland.txt $args >/dev/null 2>&1 || ok=0
        valgrind -q --error-exitcode=9 --leak-check=full "$tmp/gcc-11/route_cpp" data/lakeland.txt $args >/dev/null 2>&1 || ok=0
    done
    [ $ok = 1 ] && add "Valgrind memcheck, test suite and C/C++ route programs" "no errors, no leaks" \
                || add "Valgrind memcheck, test suite and C/C++ route programs" "REPORTED A PROBLEM"
else
    add "Valgrind memcheck, test suite and C/C++ route programs" "skipped: valgrind not installed"
fi

if command -v python3 >/dev/null 2>&1; then
    python3 tests/mutations.py gcc >"$tmp/mut.txt" 2>&1
    add "Mutation tests (13 mistakes, 1 harmless change)" "$(tail -1 "$tmp/mut.txt")"
else
    add "Mutation tests" "skipped: python3 not installed"
fi

printf '\nEnvironment: %s, %s, cmake %s, %s logical CPUs\n\n| Check | Result |\n| --- | --- |\n%s' \
    "$os" "$(gcc --version | head -1)" "$(cmake --version | head -1 | cut -d' ' -f3)" "$(nproc 2>/dev/null || echo ?)" "$rows"
if [ -s "$tmp/mut.txt" ]; then printf '\n```\n'; cat "$tmp/mut.txt"; printf '```\n'; fi

if [ -x "$tmp/gcc-11/timing" ]; then
    printf '\nTiming (gcc, Release build, -O3 from CMake defaults):\n\n```\n'
    "$tmp/gcc-11/timing" "${TIMING_MAX:-1600}"
    printf '```\n'
fi
rm -rf "$tmp"
