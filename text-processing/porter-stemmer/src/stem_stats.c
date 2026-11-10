/* stem_stats.c - what the stemmer does to a whole vocabulary: how many
 * words it changes, how many distinct stems remain, where the two modes
 * differ, and how many stems change again if stemmed a second time.
 * Usage: stem_stats VOC */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "porter.h"

#define MAX_WORDS 100000
#define WORD_SIZE 64

static int compare(const void *a, const void *b)
{
    return strcmp(a, b);
}

static size_t count_distinct(char (*list)[WORD_SIZE], size_t n)
{
    qsort(list, n, WORD_SIZE, compare);
    size_t distinct = n > 0 ? 1 : 0;
    for (size_t i = 1; i < n; ++i)
        if (strcmp(list[i], list[i - 1]) != 0)
            ++distinct;
    return distinct;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fputs("usage: stem_stats VOC\n", stderr);
        return 2;
    }
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
        perror(argv[1]);
        return 1;
    }
    static char words[MAX_WORDS][WORD_SIZE], stems[MAX_WORDS][WORD_SIZE];
    size_t n = 0, changed = 0, modes_differ = 0, not_stable = 0;
    size_t letters_in = 0, letters_out = 0;
    char line[256];
    while (n < MAX_WORDS && fgets(line, sizeof line, f) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strlen(line) >= WORD_SIZE)
            continue;
        strcpy(words[n], line);
        strcpy(stems[n], line);
        letters_in += strlen(line);
        letters_out += porter_stem(stems[n], PORTER_STANDARD);
        if (strcmp(stems[n], words[n]) != 0)
            ++changed;

        char paper[WORD_SIZE], again[WORD_SIZE];
        strcpy(paper, line);
        porter_stem(paper, PORTER_PAPER);
        if (strcmp(paper, stems[n]) != 0)
            ++modes_differ;
        strcpy(again, stems[n]);
        porter_stem(again, PORTER_STANDARD);
        if (strcmp(again, stems[n]) != 0)
            ++not_stable;
        ++n;
    }
    fclose(f);

    printf("words:                      %zu\n", n);
    printf("changed by the stemmer:     %zu (%.1f%%)\n", changed, 100.0 * (double)changed / (double)n);
    printf("letters removed:            %.1f%%\n",
           100.0 * (double)(letters_in - letters_out) / (double)letters_in);
    printf("distinct stems:             %zu\n", count_distinct(stems, n));
    printf("standard and paper differ:  %zu words\n", modes_differ);
    printf("stem changes if stemmed again: %zu words\n", not_stable);
    return 0;
}
