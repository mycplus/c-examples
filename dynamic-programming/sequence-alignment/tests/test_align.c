/* test_align.c - checks align_global, align_local and align_score against
 * known results, against a brute force that scores every possible
 * alignment, and against each other. Exits with status 1 on any failure. */
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "align.h"
#include "blosum.h"

static int failures = 0;

#define CHECK(cond, ...)                                              \
    do {                                                              \
        if (!(cond)) {                                                \
            printf("FAILED line %d: ", __LINE__);                     \
            printf(__VA_ARGS__);                                      \
            printf("\n");                                             \
            failures++;                                               \
        }                                                             \
    } while (0)

/* splitmix64: a small, well-mixed generator that gives the same numbers on
 * every platform, unlike rand(). */
static uint64_t rng_state = 2026;
static uint64_t next_random(void)
{
    uint64_t z = (rng_state += 0x9e3779b97f4a7c15u);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9u;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebu;
    return z ^ (z >> 31);
}
static int below(int n) { return (int)(next_random() % (uint64_t)n); }

static int letter(const Scoring *sc, char c)
{
    const char *p = strchr(sc->letters, toupper((unsigned char)c));
    return p == NULL ? -1 : (int)(p - sc->letters);
}

/* The score of an alignment computed column by column from the rules:
 * pairs score from the table, and every maximal run of gaps in one row
 * costs gap_open + length * gap_extend. */
static int score_columns(const char *top, const char *bottom, const Scoring *sc)
{
    size_t n = strlen(sc->letters);
    int score = 0;
    for (size_t k = 0; top[k] != '\0'; k++) {
        if (top[k] == '-' || bottom[k] == '-') {
            const char *row = top[k] == '-' ? top : bottom;
            if (k == 0 || row[k - 1] != '-')
                score -= sc->gap_open;
            score -= sc->gap_extend;
        } else {
            score += sc->table[(size_t)letter(sc, top[k]) * n + (size_t)letter(sc, bottom[k])];
        }
    }
    return score;
}

/* Brute force: the best score over every alignment of a and b, found by
 * trying all three choices for the last column at every step. Too slow for
 * anything but short strings, which is the point: it shares no code or
 * reasoning with the dynamic program. */
static char bt[32], bb[32];
static int best_seen;

static void enumerate(const char *a, size_t n, const char *b, size_t m, size_t len,
                      const Scoring *sc)
{
    if (n == 0 && m == 0) {
        char top[32], bottom[32];
        for (size_t k = 0; k < len; k++) {      /* columns were built backwards */
            top[k] = bt[len - 1 - k];
            bottom[k] = bb[len - 1 - k];
        }
        top[len] = bottom[len] = '\0';
        int s = score_columns(top, bottom, sc);
        if (s > best_seen)
            best_seen = s;
        return;
    }
    if (n > 0 && m > 0) {
        bt[len] = a[n - 1], bb[len] = b[m - 1];
        enumerate(a, n - 1, b, m - 1, len + 1, sc);
    }
    if (n > 0) {
        bt[len] = a[n - 1], bb[len] = '-';
        enumerate(a, n - 1, b, m, len + 1, sc);
    }
    if (m > 0) {
        bt[len] = '-', bb[len] = b[m - 1];
        enumerate(a, n, b, m - 1, len + 1, sc);
    }
}

static int brute_global(const char *a, size_t n, const char *b, size_t m, const Scoring *sc)
{
    best_seen = INT_MIN;
    enumerate(a, n, b, m, 0, sc);
    return best_seen;
}

/* A local alignment is a global alignment of two substrings, or empty. */
static int brute_local(const char *a, const char *b, const Scoring *sc)
{
    size_t n = strlen(a), m = strlen(b);
    int best = 0;
    for (size_t i = 0; i < n; i++)
        for (size_t i2 = i + 1; i2 <= n; i2++)
            for (size_t j = 0; j < m; j++)
                for (size_t j2 = j + 1; j2 <= m; j2++) {
                    int s = brute_global(a + i, i2 - i, b + j, j2 - j, sc);
                    if (s > best)
                        best = s;
                }
    return best;
}

/* Checks that al is a well-formed alignment of the stated parts of s1 and s2,
 * that its columns add up to its score, and that the score is `want`. */
static void check_alignment(const char *s1, const char *s2, const Scoring *sc, int local,
                            const Alignment *al, int want)
{
    char what[160];
    snprintf(what, sizeof what, "%s s1=\"%.30s\" s2=\"%.30s\" open=%d extend=%d",
             local ? "local" : "global", s1, s2, sc->gap_open, sc->gap_extend);
    size_t len = strlen(al->top);
    CHECK(strlen(al->bottom) == len, "%s: rows of different length", what);
    if (strlen(al->bottom) != len)
        return;
    /* Removing the gaps from each row must give back the aligned parts. */
    size_t k1 = 0, k2 = 0, bad1 = 0, bad2 = 0;
    for (size_t k = 0; k < len; k++) {
        CHECK(!(al->top[k] == '-' && al->bottom[k] == '-'), "%s: a column of two gaps", what);
        if (al->top[k] != '-')
            bad1 += al->start1 + k1 >= al->end1 || s1[al->start1 + k1++] != al->top[k];
        if (al->bottom[k] != '-')
            bad2 += al->start2 + k2 >= al->end2 || s2[al->start2 + k2++] != al->bottom[k];
    }
    CHECK(bad1 == 0 && al->end1 - al->start1 == k1,
          "%s: top row \"%.30s\" is not s1[%zu, %zu)", what, al->top, al->start1, al->end1);
    CHECK(bad2 == 0 && al->end2 - al->start2 == k2,
          "%s: bottom row \"%.30s\" is not s2[%zu, %zu)", what, al->bottom, al->start2, al->end2);
    if (!local)
        CHECK(al->start1 == 0 && al->end1 == strlen(s1) && al->start2 == 0
              && al->end2 == strlen(s2), "%s: a global alignment must cover both", what);
    CHECK(score_columns(al->top, al->bottom, sc) == al->score,
          "%s: columns add up to %d, reported score %d", what,
          score_columns(al->top, al->bottom, sc), al->score);
    CHECK(al->score == want, "%s: score %d, wanted %d", what, al->score, want);
}

/* Runs all three functions on one pair and checks them against `want`. */
static void check_pair(const char *s1, const char *s2, const Scoring *sc, int local, int want)
{
    Alignment al;
    int score = 0;
    int err = local ? align_local(s1, s2, sc, &al) : align_global(s1, s2, sc, &al);
    CHECK(err == ALIGN_OK, "align: %s", align_error(err));
    if (err != ALIGN_OK)
        return;
    check_alignment(s1, s2, sc, local, &al, want);
    alignment_free(&al);
    err = align_score(s1, s2, sc, local, &score);
    CHECK(err == ALIGN_OK && score == want, "align_score %s s1=\"%s\" s2=\"%s\": %d, wanted %d",
          local ? "local" : "global", s1, s2, score, want);
}

static const int DNA[16] = {3, -3, -3, -3, -3, 3, -3, -3, -3, -3, 3, -3, -3, -3, -3, 3};

static void known_results(void)
{
    const Scoring dna = {"ACGT", DNA, 0, 2};
    const Scoring dna_affine = {"ACGT", DNA, 8, 1};
    const Scoring b50 = {BLOSUM_LETTERS, BLOSUM50, 0, 8};
    const Scoring b62 = {BLOSUM_LETTERS, BLOSUM62, 11, 1};
    Alignment al;

    check_pair("TGTTACGG", "GGTTGACTA", &dna, 1, 13);
    check_pair("TGTTACGG", "GGTTGACTA", &dna, 0, 4);
    check_pair("HEAGAWGHEE", "PAWHEAE", &b50, 0, 1);
    check_pair("HEAGAWGHEE", "PAWHEAE", &b50, 1, 28);
    check_pair("GAAAAAGATAGAGC", "GAAAAAGAAAC", &dna, 0, 27);
    check_pair("GAAAAAGATAGAGC", "GAAAAAGAAAC", &dna_affine, 0, 16);

    if (align_local("HEAGAWGHEE", "PAWHEAE", &b50, &al) == ALIGN_OK) {
        CHECK(strcmp(al.top, "AWGHE") == 0 && strcmp(al.bottom, "AW-HE") == 0,
              "local BLOSUM50: got %s / %s", al.top, al.bottom);
        alignment_free(&al);
    }
    if (align_global("GAAAAAGATAGAGC", "GAAAAAGAAAC", &dna_affine, &al) == ALIGN_OK) {
        CHECK(strstr(al.bottom, "---") != NULL, "affine gaps: expected one gap of 3, got %s",
              al.bottom);
        alignment_free(&al);
    }

    /* Edge cases: empty sequences, no similarity, lowercase, bad input. */
    check_pair("", "", &dna, 0, 0);
    check_pair("", "ACG", &dna, 0, -6);
    check_pair("ACG", "", &dna_affine, 0, -11);
    check_pair("", "ACG", &dna, 1, 0);
    check_pair("AAAA", "CCCC", &dna, 1, 0);
    check_pair("acgt", "ACGT", &dna, 1, 12);
    check_pair("WWW*", "WWW*", &b62, 1, 34);
    CHECK(align_global("ACGU", "ACG", &dna, &al) == ALIGN_BAD_LETTER, "U is not in ACGT");
    CHECK(align_local("ACG", "AC\xe9", &dna, &al) == ALIGN_BAD_LETTER, "byte 0xE9 is not in ACGT");
    const Scoring negative = {"ACGT", DNA, -1, 2};
    CHECK(align_global("A", "A", &negative, &al) == ALIGN_BAD_SCORING, "negative gap open");
    const Scoring huge = {"ACGT", DNA, 1 << 20, 1 << 20};   /* one gap costs 2^21 */
    char long_a[201];
    memset(long_a, 'A', 200);
    long_a[200] = '\0';
    CHECK(align_global(long_a, "A", &huge, &al) == ALIGN_TOO_LONG, "scores could overflow");
}

/* Random scoring over a three-letter alphabet. The table is deliberately not
 * symmetric, so swapping the two sequences' roles is caught. */
static void random_scoring(Scoring *sc, int *table)
{
    for (int k = 0; k < 9; k++)
        table[k] = below(11) - 5;
    sc->letters = "ACG";
    sc->table = table;
    sc->gap_open = below(6);
    sc->gap_extend = below(4);
}

static void random_string(char *s, int max_len)
{
    int len = below(max_len + 1);
    for (int k = 0; k < len; k++)
        s[k] = "ACG"[below(3)];
    s[len] = '\0';
}

static void against_brute_force(int pairs)
{
    int table[9];
    Scoring sc;
    char a[8], b[8];
    int before = failures;
    for (int t = 0; t < pairs; t++) {
        random_scoring(&sc, table);
        random_string(a, 6);
        random_string(b, 6);
        check_pair(a, b, &sc, 0, brute_global(a, strlen(a), b, strlen(b), &sc));
        random_string(a, 5);
        random_string(b, 5);
        check_pair(a, b, &sc, 1, brute_local(a, b, &sc));
    }
    printf("random pairs matching the brute force, global and local: %d of %d\n",
           failures == before ? pairs : 0, pairs);
}

/* Longer protein pairs: align_score in linear memory must agree with the full
 * tables, and the local score can never be below the global score or 0. */
static void long_pairs(int pairs)
{
    const Scoring b62 = {BLOSUM_LETTERS, BLOSUM62, 11, 1};
    static char a[401], b[401];
    int agree = 0;
    for (int t = 0; t < pairs; t++) {
        int n = below(400), m = below(400);
        for (int k = 0; k < n; k++)
            a[k] = BLOSUM_LETTERS[below(20)];
        for (int k = 0; k < m; k++)
            b[k] = BLOSUM_LETTERS[below(20)];
        a[n] = b[m] = '\0';
        int ok = 1;
        Alignment g, l;
        int gs = 0, ls = 0;
        if (align_global(a, b, &b62, &g) != ALIGN_OK || align_local(a, b, &b62, &l) != ALIGN_OK)
            ok = 0;
        else {
            check_alignment(a, b, &b62, 0, &g, g.score);
            check_alignment(a, b, &b62, 1, &l, l.score);
            ok = align_score(a, b, &b62, 0, &gs) == ALIGN_OK && gs == g.score
                 && align_score(a, b, &b62, 1, &ls) == ALIGN_OK && ls == l.score
                 && l.score >= g.score && l.score >= 0;
            alignment_free(&g);
            alignment_free(&l);
        }
        CHECK(ok, "long pair %d: scores disagree", t);
        agree += ok;
    }
    printf("protein pairs up to 400 letters where all three functions agree: %d of %d\n",
           agree, pairs);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);   /* keep output if a sanitizer aborts */
    known_results();
    against_brute_force(5000);
    long_pairs(200);
    printf("%d failed\n", failures);
    return failures != 0;
}
