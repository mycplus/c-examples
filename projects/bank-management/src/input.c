/* input.c - every answer is read as a whole line with fgets(), then
 * checked. Nothing here uses scanf(), so a typo can never leave unread
 * characters behind to be taken as the answer to the next question.
 * Every function returns 0 at end of input, so the caller can stop cleanly. */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "input.h"

/* Reads one line into buf and removes the newline. If the line is too long,
 * the rest is thrown away so it cannot become the next answer, and buf is
 * emptied so the caller rejects it instead of using half a number.
 * Returns 1 on success, 0 at end of input. */
int read_line(char *buf, size_t size)
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
int read_int(const char *prompt, int lo, int hi, int *value)
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
int read_text(const char *prompt, char *out, size_t out_size)
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

/* Prints prompt and reads an amount of money from 0.01 to 1,000,000.00
 * into *value, as whole cents. money_parse() decides what is valid;
 * anything else (12.505, -5, 0, 1,000) prints the rule and asks again.
 * Returns 1 on success, 0 at end of input. */
int read_money(const char *prompt, Cents *value)
{
    char buf[LINE_SIZE];

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;
        if (money_parse(buf, value))
            return 1;
        printf("Please enter an amount from 0.01 to 1,000,000.00, for example 250 or 250.75.\n");
    }
}

/* Prints prompt and reads y, yes, n or no, in any case, with spaces around
 * it allowed. Sets *yes to 1 or 0. Anything else asks again.
 * Returns 1 on success, 0 at end of input. */
int read_yes_no(const char *prompt, int *yes)
{
    char buf[LINE_SIZE];

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;
        trim(buf);
        for (char *p = buf; *p != '\0'; p++)
            *p = (char)tolower((unsigned char)*p);
        if (strcmp(buf, "y") == 0 || strcmp(buf, "yes") == 0) {
            *yes = 1;
            return 1;
        }
        if (strcmp(buf, "n") == 0 || strcmp(buf, "no") == 0) {
            *yes = 0;
            return 1;
        }
        printf("Please answer y or n.\n");
    }
}

/* Prints prompt and reads a date as YYYY-MM-DD into *out. An empty answer
 * (just Enter) means today. Anything else that is not a real date asks
 * again. Returns 1 on success, 0 at end of input. */
int read_date_or_today(const char *prompt, Date today, Date *out)
{
    char buf[LINE_SIZE];

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;
        trim(buf);
        if (buf[0] == '\0') {
            *out = today;
            return 1;
        }
        if (date_parse(buf, out))
            return 1;
        printf("Please enter a date as YYYY-MM-DD, or press Enter for today.\n");
    }
}

/* Removes spaces and tabs from both ends of text, in place. */
void trim(char *text)
{
    char *p = text;
    size_t len;

    while (*p == ' ' || *p == '\t')
        p++;
    len = strlen(p);
    while (len > 0 && (p[len - 1] == ' ' || p[len - 1] == '\t'))
        len--;
    memmove(text, p, len);
    text[len] = '\0';
}
