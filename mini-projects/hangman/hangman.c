/*
 * hangman.c - the classic word game in the terminal.
 *
 * The program picks a secret word and shows one _ per letter. Guess one
 * letter at a time: a correct letter is revealed everywhere it appears,
 * a wrong one adds a part to the hanged man. Six wrong guesses lose.
 *
 * Build with GCC or Clang:
 *     gcc -std=c11 -Wall -Wextra -o hangman hangman.c
 * Build with Visual Studio (Developer Command Prompt):
 *     cl /W4 hangman.c
 * Run:
 *     ./hangman                     random word, play again as often as you like
 *     ./hangman --word PROGRAMMER   a friend sets the word; one round
 */
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LINE_SIZE  64
#define MAX_MISSES 6     /* head, body, two arms, two legs */
#define MAX_WORD   20    /* longest secret word, in letters */

/* The secret words: an array of strings. */
static const char *const words[] = {
    "ARRAY", "POINTER", "STRUCT", "COMPILER", "VARIABLE", "FUNCTION", "KEYWORD", "LIBRARY",
    "INTEGER", "BOOLEAN", "PROGRAM", "SYNTAX", "DEBUGGER", "LOOP", "STRING", "HEADER",
    "MEMORY", "OPERATOR", "RECURSION", "ALGORITHM",
};
#define WORD_COUNT (sizeof words / sizeof words[0])

/* The three rows of the gallows that change, for 0 to 6 misses. */
static const char *const man[MAX_MISSES + 1][3] = {
    { "      |", "      |", "      |" },
    { "  O   |", "      |", "      |" },
    { "  O   |", "  |   |", "      |" },
    { "  O   |", " /|   |", "      |" },
    { "  O   |", " /|\\  |", "      |" },
    { "  O   |", " /|\\  |", " /    |" },
    { "  O   |", " /|\\  |", " / \\  |" },
};

static int  read_line(char *buf, size_t size);
static int  set_word(const char *text, char *word);
static void draw_board(const char *word, const bool guessed[26], int misses);
static void draw_gallows(int misses);
static void show_word(const char *word, const bool guessed[26]);
static int  all_revealed(const char *word, const bool guessed[26]);
static int  count_letter(const char *word, char letter);
static int  read_guess(char *letter);
static int  play_round(const char *word);
static int  ask_play_again(void);

int main(int argc, char *argv[])
{
    char word[MAX_WORD + 1];

    if (argc == 3 && strcmp(argv[1], "--word") == 0) {
        if (!set_word(argv[2], word)) {
            fprintf(stderr, "hangman: the word must be 1 to %d letters (A-Z).\n", MAX_WORD);
            return 1;
        }
    } else if (argc != 1) {
        fprintf(stderr, "Usage: hangman [--word WORD]\n");
        return 1;
    }

    printf("Hangman\n=======\n");
    printf("Guess the word one letter at a time. %d wrong guesses and you lose.\n", MAX_MISSES);

    if (argc == 3) {                            /* two-player mode: one round */
        if (play_round(word) < 0)
            printf("\n\nGoodbye!\n");
        return 0;
    }

    srand((unsigned)time(NULL));
    for (;;) {
        int again;

        if (play_round(words[(size_t)rand() % WORD_COUNT]) < 0)
            break;                              /* input ended */
        again = ask_play_again();
        if (again < 0)
            break;
        if (again == 0) {
            printf("\nGoodbye!\n");
            return 0;
        }
    }
    printf("\n\nGoodbye!\n");
    return 0;
}

/* Reads one line into buf and removes the newline. If the line is too long,
 * the rest is thrown away so it cannot become the next answer, and buf is
 * emptied so the caller rejects it instead of using half a number.
 * Returns 1 on success, 0 at end of input. */
static int read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL)
        return 0;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else if (len == size - 1) {               /* buffer full: is there more? */
        int c, extra = 0;
        while ((c = getchar()) != '\n' && c != EOF)
            extra = 1;
        if (extra)
            buf[0] = '\0';
    }
    return 1;
}


/* Checks the text given with --word: 1 to MAX_WORD characters, all letters.
 * Copies it into word in uppercase, so "Programmer" becomes "PROGRAMMER".
 * Returns 1 if the word is valid, 0 otherwise. */
static int set_word(const char *text, char *word)
{
    size_t len = strlen(text);

    if (len == 0 || len > MAX_WORD)
        return 0;
    for (size_t i = 0; i <= len; i++) {         /* <= len also copies the '\0' */
        unsigned char c = (unsigned char)text[i];
        if (c != '\0' && !isalpha(c))
            return 0;
        word[i] = (char)toupper(c);
    }
    return 1;
}

/* Prints a blank line, the gallows for misses and the word with its blanks. */
static void draw_board(const char *word, const bool guessed[26], int misses)
{
    printf("\n");
    draw_gallows(misses);
    show_word(word, guessed);
}

/* Prints the gallows with the parts of the man for misses (0 to 6). */
static void draw_gallows(int misses)
{
    printf("  +---+\n  |   |\n");
    for (int row = 0; row < 3; row++)
        printf("%s\n", man[misses][row]);
    printf("      |\n=========\n");
}

/* Prints "Word:   " and each letter, or _ if it is not guessed yet,
 * separated by single spaces, with no space after the last one. */
static void show_word(const char *word, const bool guessed[26])
{
    printf("Word:   ");
    for (int i = 0; word[i] != '\0'; i++) {     /* a string ends at '\0' */
        if (i > 0)
            printf(" ");
        printf("%c", guessed[word[i] - 'A'] ? word[i] : '_');
    }
    printf("\n");
}

/* Returns 1 if every letter of word has been guessed, 0 otherwise. */
static int all_revealed(const char *word, const bool guessed[26])
{
    for (int i = 0; word[i] != '\0'; i++)
        if (!guessed[word[i] - 'A'])
            return 0;
    return 1;
}

/* Returns how many times letter appears in word. */
static int count_letter(const char *word, char letter)
{
    int n = 0;

    for (int i = 0; word[i] != '\0'; i++)
        if (word[i] == letter)
            n++;
    return n;
}

/* Prints "Guess a letter: " and reads exactly one letter, ignoring spaces
 * and tabs around it, and stores it uppercase in *letter. Anything else
 * asks again. Repeats are not checked here; play_round() does that.
 * Returns 1 on success, 0 at end of input. */
static int read_guess(char *letter)
{
    char buf[LINE_SIZE];

    for (;;) {
        char *p = buf, *end;

        printf("Guess a letter: ");
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;
        while (*p == ' ' || *p == '\t')
            p++;
        end = p + strlen(p);
        while (end > p && (end[-1] == ' ' || end[-1] == '\t'))
            end--;
        /* isalpha() and toupper() need an unsigned char value: passing a
         * negative char (such as a byte of an accented letter) is undefined. */
        if (end - p == 1 && isalpha((unsigned char)*p)) {
            *letter = (char)toupper((unsigned char)*p);
            return 1;
        }
        printf("Please type a single letter from A to Z.\n");
    }
}

/* Plays one game with the secret word (uppercase letters).
 * guessed[letter - 'A'] records each letter tried: 'A'..'Z' minus 'A' gives
 * 0..25. That relies on the letters having consecutive codes, true in ASCII
 * and on every system these programs target (C guarantees it only for '0'..'9').
 * Returns 1 for a win, 0 for a loss, -1 if input ended. */
static int play_round(const char *word)
{
    bool guessed[26] = { false };
    char missed[MAX_MISSES + 1] = "";           /* wrong letters, in order */
    int misses = 0;
    char letter = 'A';

    for (;;) {
        draw_board(word, guessed, misses);
        printf("Missed: ");
        if (misses == 0)
            printf("(none)");
        for (int i = 0; i < misses; i++)
            printf(i > 0 ? " %c" : "%c", missed[i]);
        printf("\nGuesses left: %d\n", MAX_MISSES - misses);

        if (!read_guess(&letter))
            return -1;
        if (guessed[letter - 'A']) {
            printf("You already guessed %c. Try another letter.\n", letter);
            continue;                           /* a repeat costs nothing */
        }
        guessed[letter - 'A'] = true;

        int n = count_letter(word, letter);
        if (n > 0) {
            printf("Yes! %c appears %d time%s.\n", letter, n, n == 1 ? "" : "s");
            if (all_revealed(word, guessed)) {
                draw_board(word, guessed, misses);
                printf("You win! The word was %s. Wrong guesses: %d.\n", word, misses);
                return 1;
            }
        } else {
            printf("No %c in the word.\n", letter);
            missed[misses++] = letter;
            missed[misses] = '\0';
            if (misses == MAX_MISSES) {
                draw_board(word, guessed, misses);
                printf("You lose. The word was %s.\n", word);
                return 0;
            }
        }
    }
}

/* Prints "Play again? (y/n): " and reads y, yes, n or no in any case,
 * with spaces around it allowed. Anything else asks again.
 * Returns 1 for yes, 0 for no, -1 at end of input. */
static int ask_play_again(void)
{
    char buf[LINE_SIZE];

    for (;;) {
        char *p = buf, *end;

        printf("Play again? (y/n): ");
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return -1;
        while (*p == ' ' || *p == '\t')
            p++;
        end = p + strlen(p);
        while (end > p && (end[-1] == ' ' || end[-1] == '\t'))
            end--;
        *end = '\0';                            /* cut the trailing spaces */
        for (char *q = p; *q != '\0'; q++)
            *q = (char)tolower((unsigned char)*q);
        if (strcmp(p, "y") == 0 || strcmp(p, "yes") == 0)
            return 1;
        if (strcmp(p, "n") == 0 || strcmp(p, "no") == 0)
            return 0;
        printf("Please answer y or n.\n");
    }
}
