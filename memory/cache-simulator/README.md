# Cache Simulator in C (LRU and FIFO)

[![Cache Simulator](https://github.com/mycplus/c-examples/actions/workflows/cache-simulator.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/cache-simulator.yml)

Companion code for [Cache Simulator in C: LRU and FIFO Replacement](https://www.mycplus.com/programming/c/lru-fifo-cache-implementation/) on MYCPLUS. A set-associative, write-through, write-allocate cache in C11 that reads memory traces, including Valgrind Lackey traces of real programs.

| File | What it is |
| --- | --- |
| `src/cache.h`, `src/cache.c` | The cache: configuration checks, LRU and FIFO replacement, statistics |
| `src/trace.h`, `src/trace.c` | Trace reader: `R`/`W` lines and Valgrind Lackey `L`/`S`/`M` lines |
| `src/cachesim.c` | `cachesim <cache bytes> <direct\|assoc\|assoc:N> <lru\|fifo> <block bytes> <trace\|->` |
| `src/belady.c` | Belady's anomaly: FIFO misses more with 4 lines than with 3 |
| `src/matrix_trace.c` | Prints the trace of summing a matrix by rows or by columns |
| `src/matrix_sum.c` | The same sum as a real program, to trace with Valgrind |
| `tests/` | Unit tests, small traces, expected outputs |

## Build and run

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/matrix_trace col | ./build/cachesim 32768 assoc:8 lru 64 -
valgrind --tool=lackey --trace-mem=yes --log-file=col.trace ./build/matrix_sum col
./build/cachesim 32768 assoc:8 lru 64 col.trace
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 on Linux and with MSVC `/W4 /WX` on Windows, warnings as errors.
- Unit tests: configuration checks, LRU versus FIFO, Belady's anomaly, 48-bit addresses, accesses spanning two blocks, write-through counts, and the trace parser on good and malformed lines.
- `belady`, and `cachesim` on the small traces and the matrix traces, print exactly the files in `tests/expected/`.
- Bad arguments and malformed traces are rejected with a non-zero exit status.
- Everything runs clean under AddressSanitizer and UndefinedBehaviorSanitizer.
- On Linux, a real Valgrind Lackey trace of `matrix_sum` shows more than five times as many misses in column order as in row order.
