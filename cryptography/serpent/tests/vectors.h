/* vectors.h - reads tests/vectors/serpent_nessie.txt for the tests and pitfalls. */
#ifndef SERPENT_TEST_VECTORS_H
#define SERPENT_TEST_VECTORS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t key[32];
    size_t key_len;
    uint8_t plain[16], cipher[16];
} serpent_vector;

static size_t parse_hex(const char *s, uint8_t *out, size_t max)
{
    size_t n = 0;
    while (s[0] != '\0' && s[1] != '\0' && n < max) {
        unsigned v;
        if (sscanf(s, "%2x", &v) != 1)
            break;
        out[n++] = (uint8_t)v;
        s += 2;
    }
    return n;
}

/* Loads every vector into a malloc'd array; returns the count, or 0 on error. */
static size_t load_vectors(const char *path, serpent_vector **out)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror(path);
        return 0;
    }
    size_t n = 0, cap = 0;
    serpent_vector *v = NULL;
    char line[256];
    while (fgets(line, sizeof line, f) != NULL) {
        char k[65], p[33], c[33];
        if (line[0] == '#' || sscanf(line, "%64s %32s %32s", k, p, c) != 3)
            continue;
        if (n == cap) {
            cap = cap ? cap * 2 : 256;
            serpent_vector *grown = realloc(v, cap * sizeof *v);
            if (grown == NULL) {
                free(v);
                fclose(f);
                return 0;
            }
            v = grown;
        }
        v[n].key_len = parse_hex(k, v[n].key, 32);
        if (parse_hex(p, v[n].plain, 16) != 16 || parse_hex(c, v[n].cipher, 16) != 16 ||
            v[n].key_len == 0)
            continue;
        ++n;
    }
    fclose(f);
    *out = v;
    return n;
}

#endif
