# Pixel formats, palettes and pitch

Three small C11 programs that accompany the MYCPLUS article
[DirectX Components Explained](https://www.mycplus.com/game-development/directx-components/).

| Program | Shows |
| --- | --- |
| `src/color_formats.c` | How XRGB8888 and RGB565 colours are packed, and their byte order in memory |
| `src/palette_fade.c` | An 8-bit indexed image fading to black by changing only its palette |
| `src/pitch.c` | What happens when you address a padded surface with `width * 4` instead of its pitch |

## Build and test

```sh
bash tests/run_tests.sh gcc      # or clang
bash tests/sanitizers.sh gcc
```

or with CMake:

```sh
cmake -S . -B build && cmake --build build
```

## What the build checks

The `pixel-formats` workflow compiles each program with GCC and Clang under
`-std=c11` and `-std=c17` with `-Wall -Wextra -pedantic -Werror`, runs it, and
compares the output with `tests/expected/`, which is the output printed in the
article. A second job runs all three under AddressSanitizer and
UndefinedBehaviorSanitizer. A Windows job builds with MSVC at `/W4 /WX` and
compares the same output.

The expected output of `color_formats` assumes a little-endian machine, which
every GitHub-hosted runner is.
