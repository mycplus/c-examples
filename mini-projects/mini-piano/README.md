# Mini Piano in C: Turn Notes into a WAV File

Type a tune as note names such as `C4 E4 G4 C5`, choose how long each note lasts and give a file name. The program works out each note's frequency, generates the sound wave sample by sample, and writes a standard WAV file that any media player can play. It prints a table of the notes and their frequencies, then the length of the tune and the size of the file. Type `ode` instead of notes to load the opening of Beethoven's *Ode to Joy*. This is the first project in the series that writes a binary file and the first that needs the maths library.

This project is part of [C Mini Projects for Beginners](https://www.mycplus.com/programming/c/mini-projects-in-c/#mini-piano) on MYCPLUS.

## Build

With GCC or Clang (Linux, macOS, WSL):

```sh
gcc -std=c11 -Wall -Wextra -o mini_piano mini_piano.c -lm
```

`-lm` links the maths library that holds `sin()` and `pow()`. It must come **after** the source file, because the linker only pulls from a library what the files before it need.

With Visual Studio, from a **Developer Command Prompt** (no extra library needed):

```bat
cl /W4 mini_piano.c
```

## Run

```sh
./mini_piano        # Windows: mini_piano.exe
```

Then open the WAV file it wrote in any media player.

## Sample session

```text
Mini Piano
==========
Type a tune as notes separated by spaces, for example: C4 D4 E4 F4 G4
Notes are A to G, # makes a sharp, the octave is 0 to 8 (4 if left out), R is a rest.
Type ode to load the opening of Beethoven's Ode to Joy.
Tune: C4 E4 G4 C5
Note length in milliseconds (100-2000): 400
Save as: chord.wav

Note     Frequency
C4       261.63 Hz
E4       329.63 Hz
G4       392.00 Hz
C5       523.25 Hz

Wrote 4 notes (1.60 seconds) to chord.wav: 70604 bytes.
```

## What you practice

- Turning a note name into a frequency with `math.h`: the MIDI number is `12 * (octave + 1) + semitone`, and the frequency is `440 * 2^((midi - 69) / 12)`
- Generating a sine wave sample by sample, with a short fade in and out on every note to stop clicks
- Writing a binary file with `fopen(path, "wb")`, and why the `b` matters on Windows
- Writing numbers in little-endian byte order one byte at a time with `fputc`, instead of `fwrite` of an integer or a struct
- The 44-byte WAV header, field by field
- Splitting a line into words with `strtok`, which changes the string it is given
- A `struct` for a parsed note: its name and its MIDI number, with -1 for a rest
- Linking the maths library with `-lm`, and why `M_PI` is not standard C

## Tests

`tests/session.txt`, `tests/ode.txt` and `tests/eof.txt` are typed input; the matching `.expected.txt` files are the exact output, including the file size measured after writing. They run with `ctest` from the [`mini-projects`](../) folder. The WAV files the tests write are ignored by git.

## Ideas to extend it

- Flats (`Bb4`) and a length for each note (`C4:2`).
- A square or triangle wave instead of a sine.
- Chords: add the waves of several notes, then scale the result down.
- Read the tune from a text file.
