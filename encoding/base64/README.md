# Base64

RFC 4648 base64 encoding and decoding in C, with the input validation that
most short implementations leave out.

## Source files

```text
base64/
├── CMakeLists.txt
├── README.md
├── include/
│   └── base64.h
├── src/
│   ├── base64.c          the library
│   └── base64-demo.c     command line front end
└── tests/
    └── test_base64.c     RFC 4648 vectors and validation tests
```

## Design

The library allocates nothing, keeps no global state and needs no cleanup
call, so it is safe to use from any thread. The caller supplies every
buffer and asks how big it needs to be:

```c
size_t need = b64_encoded_size(len);   /* includes the terminating NUL */
char  *out  = malloc(need);
size_t n    = b64_encode(data, len, out, need);
if (n == B64_ERROR) { /* buffer too small */ }
```

Decoding rejects invalid input rather than producing garbage:

- lengths that are not a multiple of four
- characters outside the base64 alphabet, including bytes >= 0x80
- padding anywhere except the last one or two positions
- any data following padding

## Build

### CMake

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

### Directly

```bash
cc -std=c11 -Wall -Wextra -pedantic -Iinclude \
   src/base64.c src/base64-demo.c -o base64-demo

cc -std=c11 -Wall -Wextra -pedantic -Iinclude \
   src/base64.c tests/test_base64.c -o test_base64 && ./test_base64
```

### Windows

```bat
cl /nologo /TC /W4 /WX /Iinclude src\base64.c src\base64-demo.c /Fe:base64-demo.exe
```

## Usage

```bash
./base64-demo encode "Hello World!"
SGVsbG8gV29ybGQh

./base64-demo decode "SGVsbG8gV29ybGQh"
Hello World!
```

Invalid input is reported rather than decoded:

```bash
./base64-demo decode "!!!!"
not valid base64
```

## Tests

`tests/test_base64.c` covers the seven RFC 4648 section 10 vectors in both
directions, a round trip over all 256 byte values, six classes of invalid
input, and buffer-size handling. It exits non-zero on any failure, so CI
fails on a regression.

## Automated builds

GitHub Actions builds with GCC, Clang and MSVC, warnings treated as
errors, and runs the full test suite on each. A separate job runs the
tests under AddressSanitizer and UndefinedBehaviorSanitizer.

## Related MYCPLUS content

This project accompanies the base64 article on [MYCPLUS.com](https://www.mycplus.com/).
