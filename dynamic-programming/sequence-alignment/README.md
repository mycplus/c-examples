# Pairwise Sequence Alignment in C

[![Sequence Alignment](https://github.com/mycplus/c-examples/actions/workflows/sequence-alignment.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/sequence-alignment.yml)

Companion code for [Smith-Waterman and Needleman-Wunsch in C and C++](https://www.mycplus.com/computer-science/algorithms/smith-waterman-algorithm/) on MYCPLUS: optimal global (Needleman-Wunsch) and local (Smith-Waterman) alignment of two sequences with affine gap costs, using Gotoh's three tables. The C++ version is in [mycplus/cpp-examples](https://github.com/mycplus/cpp-examples/tree/main/dynamic-programming/sequence-alignment).

| File | What it is |
| --- | --- |
| `src/align.h`, `src/align.c` | `align_global()`, `align_local()`, `align_score()` (linear memory), `alignment_print()` |
| `src/blosum.h`, `src/blosum.c` | The BLOSUM62 and BLOSUM50 protein scoring tables |
| `src/align_demo.c` | Global and local alignments of DNA and protein examples, linear and affine gaps |
| `src/seqalign.c` | `seqalign [-local] [-matrix NAME] [-open N] [-extend N] [-score] FILE1 FILE2` for FASTA files |
| `pitfalls/local_without_zero.c` | Local alignment scored with and without its two rules |
| `pitfalls/letter_index.c` | A table looked up with `toupper(c) - 'A'`, read past its end by the letter Z |
| `tests/test_align.c` | Known results, edge cases, 5,000 random pairs against a brute force, long protein pairs |
| `tests/compare_biopython.py` | 600 alignments compared with Biopython's `PairwiseAligner` |
| `tests/mutations.py` | Ten deliberate mistakes (and two harmless changes), each run against the tests |
| `bench/align_speed.c` | Time and memory of the three functions up to 4,000 letters |
| `run_all.sh`, `run_all.ps1` | Every check in one command, for Ubuntu/WSL and for Windows (MSVC) |

A gap of length k costs `open + k * extend`. Tools that charge `open` for the first gap position only (EMBOSS, Biopython) need `open + extend` in their terms for the same alignment.

## Build and run

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/align_demo
./build/seqalign -local -matrix BLOSUM50 -open 0 -extend 8 tests/data/heagawghee.fa tests/data/pawheae.fa
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 on Linux and with MSVC `/W4 /WX` on Windows, warnings as errors.
- Known textbook results; 5,000 random pairs with random, deliberately asymmetric scoring tables agree with a brute force over every alignment, global and local; the linear-memory score agrees with the full tables on long protein pairs.
- The demo, `seqalign` and the zero-floor pitfall print exactly the files in `tests/expected/`.
- On Linux: everything runs clean under AddressSanitizer and UndefinedBehaviorSanitizer, `letter_index` is caught reading past its table, 600 alignments agree with Biopython, and each mutation in `tests/mutations.py` is caught.
