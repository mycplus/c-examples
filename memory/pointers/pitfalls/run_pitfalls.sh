#!/bin/sh
# Builds and runs every deliberately broken program in this directory and prints
# what the compiler and the sanitizers say. Linux with GCC only; the programs
# are expected to fail, so this script always exits 0.
# Usage: ./run_pitfalls.sh [outdir]
out=${1:-/tmp/pointer-pitfalls}
mkdir -p "$out"
out=$(cd "$out" && pwd)
cd "$(dirname "$0")" || exit 1
cc=${CC:-gcc}
std="-std=c11 -Wall -Wextra"
asan="-g -fsanitize=address -fno-omit-frame-pointer"

run() { # title, command...
    echo "##### $1"; shift; "$@" 2>&1 | sed -e 's/==[0-9]*==/==PID==/' -e 's/0x[0-9a-f]\{6,\}/0xADDR/g' | head -${LINES_MAX:-14}
}

for n in use_after_free double_free leak heap_overflow null_deref dangling_local; do
    $cc $std $asan $n.c -o "$out/$n" 2> "$out/$n.warn"
    cat "$out/$n.warn"
    run "$n (AddressSanitizer)" env ASAN_OPTIONS=detect_stack_use_after_return=1 "$out/$n"
done

$cc $std pointer_by_value.c -o "$out/pointer_by_value" && run "pointer_by_value" "$out/pointer_by_value"
$cc $std sizeof_param.c -o "$out/sizeof_param" 2>&1 | head -8; run "sizeof_param" "$out/sizeof_param"
run "const_write (compile)" $cc $std const_write.c -o "$out/const_write"
run "void_pointer_misuse (compile, with -pedantic)" $cc $std -pedantic void_pointer_misuse.c -o "$out/void_pointer_misuse"
$cc $std declaration_style.c -o "$out/declaration_style" && run "declaration_style" "$out/declaration_style"
$cc $std literal_write.c -o "$out/literal_write" && run "literal_write" sh -c "$out/literal_write; echo exit status \$?"
$cc $std uninitialized_pointer.c -o "$out/uninitialized_pointer" && echo "##### uninitialized_pointer (compiled with no warnings)" && "$out/uninitialized_pointer"
if command -v valgrind >/dev/null 2>&1; then
    run "uninitialized_pointer (Valgrind)" valgrind -q "$out/uninitialized_pointer"
fi
exit 0
