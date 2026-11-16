/* align.h - optimal pairwise alignment of two sequences: global
 * (Needleman-Wunsch) and local (Smith-Waterman), both with affine gap costs
 * computed in O(n*m) time by Gotoh's three-table method. */
#ifndef ALIGN_H
#define ALIGN_H

#include <stddef.h>
#include <stdio.h>

/* How an alignment is scored. A gap of length k costs
 * gap_open + k * gap_extend, so gap_open = 0 gives linear gap costs. */
typedef struct {
    const char *letters;  /* the alphabet, e.g. "ACGT"; lowercase input matches too */
    const int *table;     /* score of letters[i] against letters[j] at [i * n + j] */
    int gap_open;         /* charged once per gap, 0 or more */
    int gap_extend;       /* charged for every position of a gap, 1 or more */
} Scoring;

/* One optimal alignment. top and bottom have the same length and use '-'
 * for a gap. The aligned part of s1 is s1[start1, end1), of s2 s2[start2, end2);
 * a global alignment always covers both sequences completely. */
typedef struct {
    char *top;
    char *bottom;
    int score;
    size_t start1, end1;
    size_t start2, end2;
} Alignment;

enum {
    ALIGN_OK = 0,
    ALIGN_NO_MEMORY = -1,    /* the tables did not fit in memory */
    ALIGN_BAD_LETTER = -2,   /* a sequence holds a letter the alphabet lacks */
    ALIGN_TOO_LONG = -3,     /* the score could overflow an int */
    ALIGN_BAD_SCORING = -4   /* negative or oversized gap costs or scores */
};

int align_global(const char *s1, const char *s2, const Scoring *sc, Alignment *out);
int align_local(const char *s1, const char *s2, const Scoring *sc, Alignment *out);

/* The optimal score only, in memory proportional to the length of s2. */
int align_score(const char *s1, const char *s2, const Scoring *sc, int local, int *score);

/* Prints the alignment in blocks of 60 columns: the top row, a line with
 * '|' under identical letters and '+' under other pairs that score above 0,
 * and the bottom row. */
void alignment_print(const Alignment *al, const Scoring *sc, FILE *out);
void alignment_free(Alignment *al);
const char *align_error(int code);

#endif
