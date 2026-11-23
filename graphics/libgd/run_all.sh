#!/bin/sh
# Builds and tests everything with GCC and Clang (C11 and C17), runs the
# sanitizers, Valgrind and the mutation tests, and prints a Markdown report.
# Needs LibGD:  sudo apt install libgd-dev   (or: brew install gd)
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
    "$d/pitfall_png_buffer_leak" leak >/dev/null 2>"$tmp/leak.txt"
    grep -q "Direct leak" "$tmp/leak.txt" && add "The unfreed PNG buffer is reported" "$(grep -m1 'Direct leak' "$tmp/leak.txt" | sed 's/ allocated from://')" \
                                          || add "The unfreed PNG buffer is reported" "NOT REPORTED"
    "$d/pitfall_unchecked_load" tests/files/huge.png unchecked >/dev/null 2>"$tmp/null.txt"
    grep -q "null pointer" "$tmp/null.txt" && add "The unchecked NULL image is reported" "UBSan: member access within null pointer" \
                                          || add "The unchecked NULL image is reported" "NOT REPORTED"
fi

if command -v valgrind >/dev/null 2>&1 && [ -x "$tmp/gcc-11/test_imgtools" ]; then
    ok=1
    (cd "$tmp/gcc-11" && valgrind -q --error-exitcode=9 --leak-check=full ./test_imgtools "$OLDPWD/data/downloads.txt" >/dev/null 2>&1) || ok=0
    (cd "$tmp/gcc-11" && valgrind -q --error-exitcode=9 --leak-check=full ./chart "$OLDPWD/data/downloads.txt" v.png >/dev/null 2>&1) || ok=0
    (cd "$tmp/gcc-11" && valgrind -q --error-exitcode=9 --leak-check=full ./thumbnail first.jpg v.jpg 100 >/dev/null 2>&1) || ok=0
    [ $ok = 1 ] && add "Valgrind memcheck: unit tests, chart, thumbnail" "no errors, no leaks" \
                || add "Valgrind memcheck: unit tests, chart, thumbnail" "REPORTED A PROBLEM"
fi

if command -v python3 >/dev/null 2>&1; then
    python3 tests/mutations.py gcc >"$tmp/mut.txt" 2>&1
    add "Mutation tests (12 mistakes in imgtools.c)" "$(tail -1 "$tmp/mut.txt")"
fi

printf '\nEnvironment: %s, %s, %s, cmake %s, LibGD %s\n\n| Check | Result |\n| --- | --- |\n%s' \
    "$os" "$(gcc --version | head -1)" "$(clang --version 2>/dev/null | head -1)" \
    "$(cmake --version | head -1 | cut -d' ' -f3)" "$(pkg-config --modversion gdlib 2>/dev/null)" "$rows"
if [ -s "$tmp/mut.txt" ]; then printf '\n```\n'; cat "$tmp/mut.txt"; printf '```\n'; fi
rm -rf "$tmp"
