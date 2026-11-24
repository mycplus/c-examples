/*
 * personal_diary.c - a diary kept in a plain text file.
 *
 * Write an entry stamped with today's date, read every entry, or search
 * them for a word, ignoring case. The file is the diary: close the
 * program, run it again tomorrow, and the old entries are still there.
 * Each line of the file is one entry:  2026-12-01 | Started the diary.
 *
 * Build with GCC or Clang:
 *     gcc -std=c11 -Wall -Wextra -o personal_diary personal_diary.c
 * Build with Visual Studio (Developer Command Prompt):
 *     cl /W4 personal_diary.c
 * Run:
 *     ./personal_diary                       uses diary.txt and today's date
 *     ./personal_diary --file work.txt       a second diary
 *     ./personal_diary --today 2026-12-01    pretend it is another day
 */
#define _CRT_SECURE_NO_WARNINGS   /* fopen, strerror and localtime on MSVC */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 256, not 64 as in earlier projects: the copied read_text() reads into a
 * LINE_SIZE buffer, and an entry may be 200 characters long. */
#define LINE_SIZE    256   /* read_line buffer: room for a 200-character entry */
#define ENTRY_SIZE   201   /* 200 characters + '\0' */
#define SEARCH_SIZE  41    /* 40 characters + '\0' */
#define DATE_SIZE    11    /* "YYYY-MM-DD" + '\0' */
#define FILE_LINE    320   /* one line of the diary file */
#define DEFAULT_FILE "diary.txt"
#define SEPARATOR    " | "

static int  read_line(char *buf, size_t size);
static int  read_int(const char *prompt, int lo, int hi, int *value);
static int  read_text(const char *prompt, char *out, size_t out_size);
static int  is_leap_year(int year);
static int  is_valid_date(const char *text);
static void today_string(char out[DATE_SIZE]);
static int  add_entry(const char *path, const char *date, const char *text);
static int  split_entry(char *line, char **date, char **text);
static int  contains_ignore_case(const char *text, const char *word);
static void list_entries(const char *path, const char *word);

int main(int argc, char *argv[])
{
    const char *path = DEFAULT_FILE;
    char today[DATE_SIZE] = "";
    char prompt[LINE_SIZE], text[ENTRY_SIZE], word[SEARCH_SIZE];
    int choice = 0;

    for (int i = 1; i < argc; i += 2) {         /* options come in pairs, any order */
        if (i + 1 < argc && strcmp(argv[i], "--file") == 0) {
            path = argv[i + 1];
        } else if (i + 1 < argc && strcmp(argv[i], "--today") == 0 && is_valid_date(argv[i + 1])) {
            strcpy(today, argv[i + 1]);
        } else {
            fprintf(stderr, "Usage: personal_diary [--file NAME] [--today YYYY-MM-DD]\n");
            return 1;
        }
    }
    if (today[0] == '\0')
        today_string(today);
    snprintf(prompt, sizeof prompt, "Entry for %s: ", today);

    printf("Personal Diary\n==============\n");
    printf("Diary file: %s\nToday is %s.\n", path, today);

    for (;;) {
        printf("\n1. Write an entry\n2. Read all entries\n3. Search\n0. Quit\n");
        if (!read_int("Choose an option (0-3): ", 0, 3, &choice))
            break;                              /* input ended */
        if (choice == 0) {
            printf("\nGoodbye!\n");
            return 0;
        }
        if (choice == 1) {
            if (!read_text(prompt, text, ENTRY_SIZE))
                break;
            if (add_entry(path, today, text))
                printf("Saved.\n");
        } else if (choice == 2) {
            printf("\n");
            list_entries(path, NULL);
        } else {
            if (!read_text("Search for: ", word, SEARCH_SIZE))
                break;
            printf("\n");
            list_entries(path, word);
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

/* Prints prompt and reads a whole number from lo to hi into *value.
 * Anything else (letters, an empty line, 12abc, out of range) prints a
 * message and asks again. Returns 1 on success, 0 at end of input. */
static int read_int(const char *prompt, int lo, int hi, int *value)
{
    char buf[LINE_SIZE];
    char *end;

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;
        errno = 0;
        long n = strtol(buf, &end, 10);
        while (*end == ' ' || *end == '\t')
            end++;                              /* allow trailing spaces */
        if (end != buf && *end == '\0' && errno == 0 && n >= lo && n <= hi) {
            *value = (int)n;
            return 1;
        }
        printf("Please enter a whole number from %d to %d.\n", lo, hi);
    }
}

/* Prints prompt and reads a line of text, without spaces or tabs at either
 * end, into out. It must be 1 to out_size - 1 characters; anything else
 * (an empty line, a name that is too long) asks again.
 * Returns 1 on success, 0 at end of input. */
static int read_text(const char *prompt, char *out, size_t out_size)
{
    char buf[LINE_SIZE];

    for (;;) {
        char *p = buf, *end;

        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;
        while (*p == ' ' || *p == '\t')
            p++;
        end = p + strlen(p);
        while (end > p && (end[-1] == ' ' || end[-1] == '\t'))
            end--;
        size_t len = (size_t)(end - p);
        if (len >= 1 && len <= out_size - 1) {
            memcpy(out, p, len);
            out[len] = '\0';
            return 1;
        }
        printf("Please enter 1 to %d characters.\n", (int)(out_size - 1));
    }
}

/* Returns 1 if year is a leap year, 0 otherwise. A year is a leap year if
 * it is divisible by 4, except years divisible by 100, unless they are
 * also divisible by 400: 2024 and 2000 are leap years, 1900 and 2026 not. */
static int is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

/* Returns 1 if text is a real date written YYYY-MM-DD: exactly 10
 * characters, digits and two dashes in the right places, a month from 1
 * to 12 and a day that exists in that month (29 February only in a leap
 * year). Returns 0 otherwise. */
static int is_valid_date(const char *text)
{
    static const int days_in_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (strlen(text) != 10 || text[4] != '-' || text[7] != '-')
        return 0;
    for (int i = 0; i < 10; i++)
        if (i != 4 && i != 7 && !isdigit((unsigned char)text[i]))
            return 0;

    /* Every character is a checked digit, so c - '0' gives its value. */
    int year  = (text[0] - '0') * 1000 + (text[1] - '0') * 100 + (text[2] - '0') * 10 + (text[3] - '0');
    int month = (text[5] - '0') * 10 + (text[6] - '0');
    int day   = (text[8] - '0') * 10 + (text[9] - '0');
    if (month < 1 || month > 12)
        return 0;
    int last = days_in_month[month - 1];
    if (month == 2 && is_leap_year(year))
        last = 29;
    return day >= 1 && day <= last;
}

/* Writes today's date from the computer's clock into out as YYYY-MM-DD. */
static void today_string(char out[DATE_SIZE])
{
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    if (local == NULL || strftime(out, DATE_SIZE, "%Y-%m-%d", local) == 0)
        strcpy(out, "1970-01-01");              /* the clock could not be read */
}

/* Adds one line, "date | text", to the end of the diary file.
 * Returns 1 on success; otherwise prints why and returns 0. fclose() is
 * checked too, because a write error may only show up when the file is
 * closed. */
static int add_entry(const char *path, const char *date, const char *text)
{
    FILE *f = fopen(path, "a");                 /* "a" appends; "w" would erase the whole diary every time */

    if (f == NULL) {
        printf("Could not save the entry: %s\n", strerror(errno));
        return 0;
    }
    int ok = fprintf(f, "%s%s%s\n", date, SEPARATOR, text) >= 0;
    if (fclose(f) != 0)
        ok = 0;
    if (!ok)
        printf("Could not save the entry: %s\n", strerror(errno));
    return ok;
}

/* Splits a diary line at the first " | " into *date and *text, by writing
 * '\0' over the separator. A text may contain " | " itself: only the first
 * one splits. Returns 1 if the line has the separator, 0 otherwise. */
static int split_entry(char *line, char **date, char **text)
{
    char *sep = strstr(line, SEPARATOR);

    if (sep == NULL)
        return 0;
    *sep = '\0';
    *date = line;
    *text = sep + strlen(SEPARATOR);
    return 1;
}

/* Returns 1 if word appears anywhere in text, ignoring upper and lower case.
 * strcasestr() and stristr() would do this, but they are not standard C:
 * for each starting place in text, compare the letters of word one by one. */
static int contains_ignore_case(const char *text, const char *word)
{
    for (size_t i = 0; text[i] != '\0'; i++) {
        size_t j = 0;
        while (word[j] != '\0' && text[i + j] != '\0' &&
               tolower((unsigned char)text[i + j]) == tolower((unsigned char)word[j]))
            j++;
        if (word[j] == '\0')
            return 1;                           /* every letter of word matched */
    }
    return 0;
}

/* Lists the diary: every entry when word is NULL, otherwise only entries
 * whose text contains word, ignoring case. Entries keep their number in
 * the whole diary, so a search shows the same numbers as the full list.
 * The file is read twice: once to count, so the heading can come first,
 * then again to print. A missing file just means an empty diary. */
static void list_entries(const char *path, const char *word)
{
    char line[FILE_LINE];
    char *date, *text;
    int matches = 0, skipped = 0;
    FILE *f = fopen(path, "r");

    if (f == NULL) {
        if (errno == ENOENT)
            printf("Your diary is empty.\n");
        else
            printf("Could not open the diary: %s\n", strerror(errno));
        return;
    }
    for (int pass = 1; pass <= 2; pass++) {
        int number = 0;
        rewind(f);
        while (fgets(line, sizeof line, f) != NULL) {
            line[strcspn(line, "\r\n")] = '\0';    /* drop the newline (and a CR from Windows) */
            if (!split_entry(line, &date, &text)) {
                if (pass == 1)
                    skipped++;
                continue;
            }
            number++;
            if (word != NULL && !contains_ignore_case(text, word))
                continue;
            if (pass == 1)
                matches++;
            else
                printf("%3d. %s  %s\n", number, date, text);
        }
        if (pass == 2)
            break;
        if (word == NULL && matches == 0)
            printf("Your diary is empty.\n");
        else if (word == NULL)
            printf("Your diary (%d entr%s)\n", matches, matches == 1 ? "y" : "ies");
        else if (matches == 0)
            printf("No entries contain \"%s\".\n", word);
        else
            printf("%d %s \"%s\":\n", matches, matches == 1 ? "entry contains" : "entries contain", word);
    }
    if (word == NULL && skipped > 0)
        printf("(%d line%s skipped: not in diary format)\n", skipped, skipped == 1 ? "" : "s");
    if (fclose(f) != 0)
        printf("Could not close the diary: %s\n", strerror(errno));
}
