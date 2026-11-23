# Hangman Game in C

The classic word game in the terminal. The program picks a secret word and shows one `_` per letter; you guess one letter at a time. A correct letter is revealed everywhere it appears, and a wrong one adds a part to the hanged man. Six wrong guesses lose the game, and revealing every letter wins it. In normal play the word is chosen at random from a built-in list and you can play again; with `--word` a friend sets the word, which makes it a two-player game (and is how the tests run).

This project is part of [C Mini Projects for Beginners](https://www.mycplus.com/programming/c/mini-projects-in-c/#hangman) on MYCPLUS.

## Build

With GCC or Clang (Linux, macOS, WSL):

```sh
gcc -std=c11 -Wall -Wextra -o hangman hangman.c
```

With Visual Studio, from a **Developer Command Prompt**:

```bat
cl /W4 hangman.c
```

## Run

```sh
./hangman                      # random word; asks to play again
./hangman --word PROGRAMMER    # a friend sets the word; one round
```

On Windows, run `hangman.exe` the same way.

## Sample game

```text
Hangman
=======
Guess the word one letter at a time. 6 wrong guesses and you lose.

  +---+
  |   |
      |
      |
      |
      |
=========
Word:   _ _ _ _ _ _ _ _ _ _
Missed: (none)
Guesses left: 6
Guess a letter: r
Yes! R appears 3 times.

  +---+
  |   |
      |
      |
      |
      |
=========
Word:   _ R _ _ R _ _ _ _ R
Missed: (none)
Guesses left: 6
Guess a letter: x
No X in the word.

  +---+
  |   |
  O   |
      |
      |
      |
=========
Word:   _ R _ _ R _ _ _ _ R
Missed: X
Guesses left: 5
Guess a letter: 
```

## What you practice

- Strings as arrays of `char` ending in `'\0'`: loops run while `word[i] != '\0'`
- An array of strings for the word list, and a 2D table of strings for the gallows
- A lookup array indexed by letter: `bool guessed[26]` with `guessed[letter - 'A']`
- Nested loops and counting how often a letter appears
- `toupper()` and `isalpha()` with the `(unsigned char)` cast, because a negative `char` is undefined behaviour for them
- The ternary operator for plurals: `n == 1 ? "" : "s"`
- Command-line arguments with `argc`/`argv`, including checking the `--word` value
- `rand()` and `srand(time(NULL))` to choose a word

## Tests

`tests/win.txt`, `tests/lose.txt` and `tests/eof.txt` are typed input; the matching `.expected.txt` files are the exact output. All three use `--word`, never `rand()`, because `rand()` gives different sequences on different C libraries. They run with `ctest` from the [`mini-projects`](../) folder.

## Ideas to extend it

- Let the player guess the whole word at once.
- Read the word list from a text file.
- Add difficulty levels by word length.
- Hide the `--word` value by reading it with no echo (platform-specific, a good next step).
