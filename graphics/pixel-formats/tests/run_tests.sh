#!/usr/bin/env bash
# Usage: tests/run_tests.sh <compiler>   e.g. gcc, clang
# Builds each example with warnings as errors, runs it and compares its
# output with the captured output published in the article.
set -euo pipefail
CC=${1:-cc}
here=$(cd "$(dirname "$0")/.." && pwd)
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
for name in color_formats palette_fade pitch; do
    for std in c11 c17; do
        "$CC" -std=$std -Wall -Wextra -pedantic -Werror -o "$out/$name" "$here/src/$name.c"
        "$out/$name" > "$out/$name.txt"
        diff -u "$here/tests/expected/$name.txt" "$out/$name.txt"
        echo "ok  $CC -std=$std $name"
    done
done
