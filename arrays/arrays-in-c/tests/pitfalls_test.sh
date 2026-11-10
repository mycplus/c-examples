#!/bin/sh
# Checks that each deliberately broken program still fails the way the article says.
# Linux with GCC or Clang.  Usage: sh tests/pitfalls_test.sh CC
cc=${1:-cc}
dir=$(cd "$(dirname "$0")/../pitfalls" && pwd)
tmp=$(mktemp -d)
status=0
export LC_ALL=C

$cc -std=c11 -c "$dir/array_assign.c" -o "$tmp/a.o" 2>"$tmp/assign.txt" && status=1
grep -q "array type" "$tmp/assign.txt" || { echo "FAIL: array assignment did not give the expected error"; status=1; }

$cc -std=c11 -Wall -Wextra "$dir/array_compare.c" -o "$tmp/cmp" 2>"$tmp/cmp.txt"
grep -Eq "Warray-compare|array comparison" "$tmp/cmp.txt" || { echo "FAIL: no warning for == on arrays"; status=1; }
"$tmp/cmp" | grep -q "not equal" || { echo "FAIL: == on arrays did not compare addresses"; status=1; }

$cc -std=c11 -g -fsanitize=address "$dir/out_of_bounds.c" -o "$tmp/oob" 2>/dev/null &&
    { "$tmp/oob" 2>"$tmp/oob.txt"; grep -q "stack-buffer-overflow" "$tmp/oob.txt" ||
      { echo "FAIL: AddressSanitizer did not report the out-of-bounds read"; status=1; }; }

rm -rf "$tmp"
[ $status = 0 ] && echo "pitfalls: all checks passed"
exit $status
