/* screen.c - redraw only the characters that changed between two frames,
 * using ANSI cursor positioning. */
#include "screen.h"

#include <stdio.h>
#include <string.h>

static void append(char *out, size_t cap, size_t *n, const char *s, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        if (*n + 1 < cap)
            out[*n] = s[i];
        (*n)++;
    }
}

size_t screen_diff(const char *prev, const char *cur, char *out, size_t cap)
{
    size_t n = 0;
    if (prev == NULL) {
        append(out, cap, &n, "\x1b[H\x1b[2J", 7);     /* home, clear */
        for (const char *p = cur; *p != '\0'; ++p) {
            if (*p == '\n')
                append(out, cap, &n, "\r\n", 2);    /* raw mode: add CR */
            else
                append(out, cap, &n, p, 1);
        }
    } else {
        int row = 1, col = 1;                        /* ANSI is 1-based */
        const char *a = prev, *b = cur;
        while (*b != '\0') {
            if (*b == '\n') {
                row++;
                col = 1;
                if (*a != '\0') a++;
                b++;
                continue;
            }
            if (*a == *b) {                          /* unchanged character */
                col++;
                a++;
                b++;
                continue;
            }
            char move[32];                           /* start of a changed run */
            int len = snprintf(move, sizeof move, "\x1b[%d;%dH", row, col);
            append(out, cap, &n, move, (size_t)len);
            while (*b != '\0' && *b != '\n' && *a != *b) {
                append(out, cap, &n, b, 1);
                col++;
                if (*a != '\0' && *a != '\n') a++;
                b++;
            }
        }
    }
    if (cap > 0)
        out[n < cap ? n : cap - 1] = '\0';
    return n;
}
