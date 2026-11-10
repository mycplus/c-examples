#!/usr/bin/env bash
# run_all.sh - every check behind the article's "What I Verified" table, on
# Ubuntu or WSL. Run from this directory:  bash run_all.sh
# Prints one PASS/FAIL/SKIP line per row; paste them into the table.
set -u
cd "$(dirname "$0")"
results=()
row() { results+=("$(printf '%-44s %s' "$1" "$2")"); }
run() { local name=$1; shift; if "$@" >/tmp/btree_run_all.log 2>&1; then row "$name" PASS; else row "$name" FAIL; cat /tmp/btree_run_all.log; fi; }

if ! command -v cmake >/dev/null; then echo "cmake is required (sudo apt install cmake)"; exit 2; fi
for cc in gcc clang; do
  if ! command -v "$cc" >/dev/null; then row "Build and CTest, $cc" "SKIP (not installed)"; continue; fi
  for std in 11 17; do
    run "Build and CTest, $cc C$std" bash -c "rm -rf build-$cc-$std && cmake -S . -B build-$cc-$std -DCMAKE_C_COMPILER=$cc -DCMAKE_C_STANDARD=$std >/dev/null && cmake --build build-$cc-$std && ctest --test-dir build-$cc-$std --output-on-failure"
  done
done
run "AddressSanitizer + UBSan (gcc), CTest" bash -c "rm -rf build-asan && cmake -S . -B build-asan -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-sanitize-recover=all -g' >/dev/null && cmake --build build-asan && ctest --test-dir build-asan --output-on-failure"
if command -v valgrind >/dev/null; then
  run "Valgrind, full test suite (about 2 minutes)" valgrind -q --error-exitcode=9 --leak-check=full build-gcc-11/test_btree
else row "Valgrind, full test suite" "SKIP (not installed)"; fi
if command -v python3 >/dev/null; then
  run "Mutation tests (9 deliberate mistakes)" python3 tests/mutations.py gcc
else row "Mutation tests (9 deliberate mistakes)" "SKIP (python3 not installed)"; fi

echo
echo "Environment: $(uname -sr), $(grep -s PRETTY_NAME /etc/os-release | cut -d'"' -f2)"
grep -qi microsoft /proc/version 2>/dev/null && echo "Running under WSL"
gcc --version 2>/dev/null | head -1
clang --version 2>/dev/null | head -1
printf '%s\n' "${results[@]}"
printf '%s\n' "${results[@]}" | grep -q FAIL && exit 1 || exit 0
