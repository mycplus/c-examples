#ifndef PATTERNS_H
#define PATTERNS_H

#include <stdio.h>

/* Every pattern takes a size n and writes whole lines to out. No line has
   trailing spaces, so the output can be compared byte for byte. */
typedef void (*pattern_fn)(FILE *out, int n);

typedef struct {
    const char *name;
    pattern_fn draw;
    int max_n;                 /* largest n that still prints sensibly */
    const char *about;
} pattern;

/* The table of all patterns, ending with an entry whose name is NULL. */
extern const pattern patterns[];

/* Finds a pattern by name, or returns NULL. */
const pattern *pattern_find(const char *name);

/* Row r of Pascal's triangle, entry k, as a 64-bit unsigned value.
   Returns 0 if the value does not fit (it does for every row up to 67). */
unsigned long long pascal_entry(int r, int k);

#endif
