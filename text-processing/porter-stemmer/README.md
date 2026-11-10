# Porter Stemmer in C

[![Porter Stemmer](https://github.com/mycplus/c-examples/actions/workflows/porter-stemmer.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/porter-stemmer.yml)

Companion code for [Porter Stemmer in C](https://www.mycplus.com/computer-science/algorithms/porter-stemmer/) on MYCPLUS: the Porter stemming algorithm written in C11 from the 1980 paper's rules, with a mode for Martin Porter's own reference behaviour and a mode for the paper exactly, checked word for word against 42,603 published test words.

| File | What it is |
| --- | --- |
| `src/porter.h`, `src/porter.c` | The stemmer: `porter_stem()`, a traced version that reports every step, and `porter_measure()` |
| `src/stem.c` | `stem [--paper] [file]` stems one word per line; `stem --trace WORD` shows each step |
| `src/stem_demo.c` | Stems a few word families and shows the three differences between the modes |
| `src/stem_stats.c` | What the stemmer does to the whole test vocabulary |
| `tests/test_porter.c` | The paper's measure examples, edge cases, and all 42,603 words in both modes |
| `tests/mutations.py` | Nine deliberate mistakes, each built and checked against the vocabulary |
| `tests/compare_nltk.py` | Word-by-word comparison with NLTK's independent Python implementation |
| `bench/` | Speed in C, and NLTK's speed on the same words |
| `run_all.sh`, `run_all.ps1` | Every check in one command, for Ubuntu/WSL and for Windows (MSVC) |

`tests/vectors/` holds the test vocabulary and expected outputs; see `tests/vectors/COPYING` for where they come from and their licence.

## Build and run

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/stem_demo
./build/stem --trace generalizations
echo running | ./build/stem
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 on Linux and with MSVC `/W4 /WX` on Windows, warnings as errors.
- All 42,603 test words stem exactly as published, in both modes: the paper's output from the Snowball project's test data, and the output of Martin Porter's reference C implementation.
- The demo, the traces and the statistics print exactly the files in `tests/expected/`; bad command lines are rejected.
- On Linux: everything runs clean under AddressSanitizer and UndefinedBehaviorSanitizer, the nine mutations in `tests/mutations.py` each change at least one stem, and NLTK's implementation agrees word for word.
