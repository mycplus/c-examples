#!/usr/bin/env bash
# run_all.sh - every check behind the article's "What I Verified" table, on
# Ubuntu or WSL. Run from this directory:  sh run_all.sh  (or bash run_all.sh)
# Prints one PASS/FAIL/SKIP line per row and writes them to results-wsl.md
# (under WSL) or results-linux.md. A failing step's output is saved in
# run_all-<step>.log next to this script.
set -u
cd "$(dirname "$0")"
results=()
row() { results+=("$(printf '%-48s %s' "$1" "$2")"); }
run() {
  local name=$1 log; shift
  log="run_all-$(echo "$name" | tr -c 'A-Za-z0-9\n' '-' | tr -s '-' | cut -c1-40).log"
  if "$@" >"$log" 2>&1; then row "$name" PASS; rm -f "$log"; else row "$name" "FAIL (see $log)"; fi
}

if ! command -v cmake >/dev/null; then echo "cmake is required (sudo apt install cmake)"; exit 2; fi
for cc in gcc clang; do
  if ! command -v "$cc" >/dev/null; then row "Build and CTest, $cc" "SKIP (not installed)"; continue; fi
  for std in 11 17; do
    run "Build and CTest, $cc C$std" bash -c "rm -rf build-$cc-$std && cmake -S . -B build-$cc-$std -DCMAKE_C_COMPILER=$cc -DCMAKE_C_STANDARD=$std && cmake --build build-$cc-$std && ctest --test-dir build-$cc-$std --output-on-failure"
  done
done
run "AddressSanitizer + UBSan (gcc), CTest" bash -c "rm -rf build-asan && cmake -S . -B build-asan -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-sanitize-recover=all -g' && cmake --build build-asan && ctest --test-dir build-asan --output-on-failure"
if command -v valgrind >/dev/null; then
  run "Valgrind, full test suite" valgrind -q --error-exitcode=9 --leak-check=full build-gcc-11/test_linear_search
else row "Valgrind, full test suite" "SKIP (not installed)"; fi
run "off_by_one heap read caught by ASan" bash -c "! build-asan/pitfall_off_by_one heap >/dev/null 2>run_all-obo.txt && grep -q 'AddressSanitizer: heap-buffer-overflow' run_all-obo.txt; s=\$?; rm -f run_all-obo.txt; exit \$s"
if command -v python3 >/dev/null; then
  run "Mutation tests (9 mistakes, 1 harmless change)" python3 tests/mutations.py gcc
else row "Mutation tests (9 mistakes, 1 harmless change)" "SKIP (python3 not installed)"; fi

out=results-linux.md
grep -qi microsoft /proc/version 2>/dev/null && out=results-wsl.md
{
  echo "Environment: $(uname -sr), $(grep -s PRETTY_NAME /etc/os-release | cut -d'"' -f2)"
  grep -qi microsoft /proc/version 2>/dev/null && echo "Running under WSL"
  gcc --version 2>/dev/null | head -1
  clang --version 2>/dev/null | head -1
  printf '%s\n' "${results[@]}"
} | tee "$out"
printf '%s\n' "${results[@]}" | grep -q FAIL && exit 1 || exit 0
