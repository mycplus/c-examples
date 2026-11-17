#!/bin/sh
# Builds and tests everything with GCC and Clang (C11 and C17, plus C++17),
# runs the sanitizers and the mutation tests, and prints a Markdown report.
# Works on Ubuntu and on WSL. Needs cmake, gcc, g++ and python3; clang and
# valgrind are used if present.
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
            c23=$(grep -q "HAVE_C23_ENUM:INTERNAL=1" "$d/CMakeCache.txt" && echo "C23 enum built" || echo "no C23 enum")
            add "$cc, C$std + C++17, warnings as errors, ctest" "${res:-no ctest result} ($c23)"
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
    if "$d/pitfall_out_of_range" unchecked >/dev/null 2>"$tmp/oor.txt"; then
        add "Unchecked enum value used as an index, ASan + UBSan" "NOT DETECTED"
    else
        add "Unchecked enum value used as an index, ASan + UBSan" "$(grep -oE "AddressSanitizer: [a-z-]+|runtime error: .*" "$tmp/oor.txt" | head -1)"
    fi
else
    add "AddressSanitizer + UBSan (gcc), ctest" "BUILD FAILED"
fi

if command -v valgrind >/dev/null 2>&1 && [ -x "$tmp/gcc-11/test_enums" ]; then
    ok=1
    for p in test_enums basics flags log_levels pitfall_reorder enum_class; do
        valgrind -q --error-exitcode=9 --leak-check=full "$tmp/gcc-11/$p" >/dev/null 2>&1 || ok=0
    done
    valgrind -q --error-exitcode=9 --leak-check=full "$tmp/gcc-11/order_demo" data/events.txt >/dev/null 2>&1 || ok=0
    [ $ok = 1 ] && add "Valgrind memcheck, every program" "no errors, no leaks" \
                || add "Valgrind memcheck, every program" "REPORTED A PROBLEM"
else
    add "Valgrind memcheck, every program" "skipped: valgrind not installed"
fi

if command -v python3 >/dev/null 2>&1; then
    python3 tests/mutations.py gcc >"$tmp/mut.txt" 2>&1
    add "Mutation tests (8 mistakes in the order library)" "$(tail -1 "$tmp/mut.txt")"
fi

for cc in gcc clang; do
    command -v "$cc" >/dev/null 2>&1 || continue
    add "underlying, $cc" "$("$tmp/$cc-11/underlying" | tr '\n' ';' | sed 's/;$//; s/;/; /g')"
done

printf '\nEnvironment: %s, %s, %s, cmake %s\n\n| Check | Result |\n| --- | --- |\n%s' \
    "$os" "$(gcc --version | head -1)" "$(clang --version 2>/dev/null | head -1)" \
    "$(cmake --version | head -1 | cut -d' ' -f3)" "$rows"
rm -rf "$tmp"
