# Prime numbers in C

[![prime-numbers](https://github.com/mycplus/c-examples/actions/workflows/prime-numbers.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/prime-numbers.yml)

Companion code for [Prime Number Programs in C, C++, Java, Python, C#, PHP and JavaScript](https://www.mycplus.com/computer-science/algorithms/prime-number-program/) on MYCPLUS: a primality test by trial division up to the square root, and the Sieve of Eratosthenes. The same program exists in seven languages and every version prints the same output.

| File | What it is |
| --- | --- |
| `src/primes.c` | `is_prime()`, `sieve()` and the demo |
| `src/divisions.c` | Counts the remainders four trial-division bounds need for 2147483647 |
| `src/benchmark.c` | Times trial division against the sieve (default: primes below 10,000,000) |
| `tests/` | Unit tests and expected output |

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/benchmark 10000000
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 and with MSVC, warnings as errors.
- Trial division agrees with the sieve on every number below 200,000.
- Known primes (including 2147483647, 1000000007 and 999999999989) and composites (including negatives, 0, 1, squares of primes and Carmichael numbers 561 and 1105) are classified correctly.
- The sieve reproduces the published prime counts: 168 below 1,000, 78,498 below 1,000,000 and 664,579 below 10,000,000.
- The demo prints exactly `tests/expected/primes.txt`, the same file in all seven repositories.
- `divisions` prints the counts shown in the article (1,073,741,822 / 46,339 / 23,170 / 15,448).
- Tests and demo run clean under AddressSanitizer and UndefinedBehaviorSanitizer.

Benchmark timings are not checked; only the benchmark's agreement check is.
