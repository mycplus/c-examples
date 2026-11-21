# MD5, SHA-1 and SHA-256 in C

[![Hash functions](https://github.com/mycplus/c-examples/actions/workflows/hash-functions.yml/badge.svg)](https://github.com/mycplus/c-examples/actions/workflows/hash-functions.yml)

Companion code for [MD5, SHA-1 and SHA-256: Hash Functions in C, C++, Java, Python and C#](https://www.mycplus.com/computer-science/algorithms/md5-sha1-sha256-hash-functions/) on MYCPLUS: readable implementations of the three hashes and HMAC-SHA-256, tested against published vectors and OpenSSL, with a collision demo and a length-extension attack.

> For learning and testing. Never store passwords with a plain hash, and never build your own cryptography for real data: use a maintained library (OpenSSL, the platform's own crypto API) and, for passwords, a slow password hash such as Argon2id, scrypt or bcrypt.

| File | What it is |
| --- | --- |
| `src/digest.h`, `src/digest.c` | MD5, SHA-1, SHA-256 and HMAC-SHA-256 with one streaming interface |
| `src/digest_demo.c` | Hashes three short messages with all three algorithms |
| `src/collisions.c` | Two inputs with the same MD5 (built in) and two with the same SHA-1 (from files) |
| `src/length_extension.c` | Forges a signature for `H(secret || message)` with SHA-256 and MD5; shows that HMAC resists it |
| `tests/test_vectors.c` | Known answers, 306 message lengths, every split offset up to 130 bytes, HMAC, resuming from a digest |
| `tests/test_against_openssl.c` | 5,000 random messages and keys compared with OpenSSL (built when OpenSSL is installed) |
| `tests/mutations.py` | Breaks `digest.c` 27 ways, one at a time, and confirms the tests notice |
| `pitfalls/` | Hex printing through a signed `char`, `strlen()` on binary data, a trailing newline |
| `bench/digest_speed.c` | Throughput over 64 MiB, compared with OpenSSL when available |
| `gen/make_vectors.py`, `gen/tables.py` | `python3 gen/make_vectors.py tests/vectors` regenerates the vector files from Python's `hashlib` after asserting the published digests; `tables.py` computes the MD5 and SHA-256 constant tables (from sines and prime roots) into `tables_c.txt` |

The vectors are in `tests/vectors/`: the RFC 1321 and FIPS 180-4 examples, 306 message lengths generated with Python's `hashlib` (an independent implementation), and the RFC 4231 and RFC 2202 HMAC cases. `tests/vectors/` is identical in all five languages. The two 640-byte SHA-1 collision files in `tests/vectors/` come from the [sha1collisiondetection](https://github.com/cr-marcstevens/sha1collisiondetection) test directory (MIT licence), published with ["SHA-1 is a Shambles"](https://sha-mbles.github.io/) by Leurent and Peyrin.

## Build and run

OpenSSL is optional; without it, the comparison test and the benchmark's OpenSSL column are skipped.

```sh
sudo apt install libssl-dev    # Debian/Ubuntu, optional
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/length_extension
./build/digest_speed           # Linux and macOS
python3 tests/mutations.py     # needs gcc or cc
```

## What the build checks

- Compiles with GCC and Clang under C11 and C17 on Linux and with MSVC `/W4 /WX` on Windows, warnings as errors.
- Every vector passes; hashing in 7-byte chunks and splitting at every offset gives the same digest as hashing in one call.
- On Linux, 20,000 digests and MACs match OpenSSL's.
- The demo, the collision demo, the length-extension attack and the three pitfalls print exactly the files in `tests/expected/`.
- Everything runs clean under AddressSanitizer and UndefinedBehaviorSanitizer.
