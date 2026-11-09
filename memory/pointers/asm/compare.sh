#!/bin/sh
# Compiles the index and pointer versions of sum() to assembly and compares the
# instructions of the function itself (labels, directives and notes are dropped).
# Usage: ./compare.sh [gcc|clang] [-O0|-O2|-O3]
cc=${1:-gcc}
opt=${2:--O2}
for v in index pointer; do
    $cc -std=c11 $opt -S -fno-asynchronous-unwind-tables -fno-ident -o - sum_$v.c \
        | awk '/^sum:/{on=1; next} /\.size/{on=0} on' \
        | grep -v -e '^\s*\.' -e '^\.L' -e '^\s*#' \
        | sed 's/^\s*//' > "sum_$v.$cc$opt.s"
done
if diff -u "sum_index.$cc$opt.s" "sum_pointer.$cc$opt.s" > "sum.$cc$opt.diff"; then
    echo "$cc $opt: identical instructions ($(wc -l < "sum_index.$cc$opt.s") each)"
else
    echo "$cc $opt: instructions differ (index $(wc -l < "sum_index.$cc$opt.s"), pointer $(wc -l < "sum_pointer.$cc$opt.s"))"
fi
