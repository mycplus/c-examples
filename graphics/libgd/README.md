# Dynamic Image Creation in C with LibGD

Example programs for the article
[LibGD in C: Create PNG and JPEG Images Dynamically](https://www.mycplus.com/programming/c/libgd-dynamic-image-creation/).

| File | What it does |
| --- | --- |
| `src/first_image.c` | The smallest useful LibGD program: draws shapes and text, saves PNG and JPEG |
| `src/chart.c` | Draws a bar chart PNG from a data file, with an optional TrueType title |
| `src/thumbnail.c` | Makes a thumbnail that keeps the aspect ratio; checks the size in the file header before decoding |
| `src/identicon.c` | Draws a default avatar from a user name |
| `src/imgtools.c` | The helpers the programs share |
| `src/imgcheck.c` | Reads an image back and prints its format, size and chosen pixels (used by the tests) |
| `pitfalls/` | Small programs that show common LibGD mistakes |

## Install LibGD

- Ubuntu and Debian: `sudo apt install libgd-dev`
- macOS: `brew install gd`
- Windows (vcpkg): `vcpkg install "libgd[core,png,jpeg,freetype]:x64-windows"`

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

On Windows, add `-DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake` to the first command.
`sh run_all.sh` (Linux) and `.\run_all.ps1` (Windows) run every check and print a report.
