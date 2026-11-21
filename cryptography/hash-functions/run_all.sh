#!/bin/sh
# Build and test everything. Call it as: sh run_all.sh   (the executable bit is lost in zip files)
set -e
for std in 11 17; do
  rm -rf "build-c$std"
  cmake -S . -B "build-c$std" -DCMAKE_C_STANDARD=$std
  cmake --build "build-c$std"
  ctest --test-dir "build-c$std" --output-on-failure
done
python3 tests/mutations.py
