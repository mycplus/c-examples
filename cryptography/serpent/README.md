# Serpent Block Cipher in C

[![Serpent](https://github.com/mycplus/c-examples/actions/workflows/serpent.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/serpent.yml)

Companion code for [Serpent Cipher Explained](https://www.mycplus.com/computer-science/algorithms/serpent-cipher/) on MYCPLUS: a readable bitslice implementation of the Serpent block cipher (128-bit blocks, keys up to 256 bits), checked against 1,043 published test vectors and against libgcrypt.

> For learning and testing. This code has not been reviewed for side channels. To protect real data, use a maintained library and an authenticated mode such as AES-GCM or ChaCha20-Poly1305.

| File | What it is |
| --- | --- |
| `src/serpent.h`, `src/serpent.c` | Key schedule, encryption and decryption of one 16-byte block |
| `src/serpent_demo.c` | Encrypts one block with a 256-bit key and checks two known vectors |
| `tests/test_vectors.c` | Runs every vector in `tests/vectors/serpent_nessie.txt` in both directions |
| `tests/test_against_libgcrypt.c` | 9,000 random keys and blocks compared with libgcrypt (built when libgcrypt is installed) |
| `pitfalls/round_trip_only.c` | A broken key schedule that still decrypts its own output |
| `pitfalls/word_byte_order.c` | Big-endian word loading, which matches none of the vectors |
| `pitfalls/ecb_patterns.c` | Why encrypting block by block (ECB) leaks repeated plaintext |
| `bench/serpent_speed.c` | Throughput over 16 MiB, compared with libgcrypt when available |

The test vectors are the NESSIE set as distributed with [Botan](https://github.com/randombit/botan) (`src/tests/data/block/serpent.vec`).

## Build and run

libgcrypt is optional; without it, the comparison test and the benchmark's comparison column are skipped.

```sh
sudo apt install libgcrypt20-dev    # Debian/Ubuntu, optional
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/serpent_demo
./build/serpent_speed               # Linux and macOS
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 on Linux and with MSVC `/W4 /WX` on Windows, warnings as errors.
- All 1,043 vectors (128-, 192- and 256-bit keys) encrypt and decrypt correctly; keys of 0 and 33 bytes are rejected.
- On Linux, 9,000 random keys and blocks give the same ciphertext as libgcrypt.
- The demo and the three pitfalls print exactly the files in `tests/expected/`.
- Everything runs clean under AddressSanitizer and UndefinedBehaviorSanitizer.
