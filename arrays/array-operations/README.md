# Array Operations (C)

Access and update, traversal, linear search, insertion, deletion, array
size and multi-dimensional arrays, using built-in C arrays.

```text
array-operations/
├── CMakeLists.txt
├── include/array_ops.h
├── src/
│   ├── array_ops.c      search, insert, remove, print
│   └── array-demo.c     each operation run once
└── tests/test_array_ops.c
```

## What the code is careful about

- **An array does not know its own length.** Every function takes the
  element count, and the functions that change it take a pointer to the
  count and the capacity, because an array cannot grow.
- **`size_t` for counts and indices.** `array_find` reports the index
  through a parameter rather than returning `-1`, which is not a value an
  unsigned type can hold.
- **`memmove`, not `memcpy`,** for the shifts in insert and remove: the
  source and destination ranges overlap.
- **Bounds are checked at the edges.** Inserting beyond the count,
  inserting at capacity and removing from an empty array are refused.
- **The column count is part of a 2D parameter type**, because the
  compiler needs it to compute where each row starts.

## Build and run

```bash
cc -std=c11 -Wall -Wextra -pedantic -Iinclude src/array_ops.c src/array-demo.c -o array-demo
cc -std=c11 -Wall -Wextra -pedantic -Iinclude src/array_ops.c tests/test_array_ops.c -o test_array_ops
./array-demo
./test_array_ops
```

Or with CMake:

```bash
cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure
```

## Automated builds

GitHub Actions builds with GCC and Clang on Ubuntu at C11 and C17 and with
Apple Clang on macOS, warnings treated as errors, then runs the test suite
and checks the demo reports every operation. Separate jobs cover the CMake
build and a run under AddressSanitizer and UndefinedBehaviorSanitizer.
