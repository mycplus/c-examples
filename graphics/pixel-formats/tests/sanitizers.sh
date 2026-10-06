#!/usr/bin/env bash
# Runs every example under AddressSanitizer and UndefinedBehaviorSanitizer.
set -euo pipefail
CC=${1:-gcc}
here=$(cd "$(dirname "$0")/.." && pwd)
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
for name in color_formats palette_fade pitch; do
    "$CC" -std=c11 -g -fsanitize=address,undefined -fno-sanitize-recover=all \
          -o "$out/$name" "$here/src/$name.c"
    "$out/$name" > "$out/$name.txt"
    diff -u "$here/tests/expected/$name.txt" "$out/$name.txt"
    echo "ok  sanitizers $name"
done
