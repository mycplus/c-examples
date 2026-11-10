/* test_lcs.c - edge cases, a brute-force check of every pair of short
 * random strings, larger random strings, and the line diff. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lcs.h"

static int failures;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            printf("FAILED line %d: %s\n", __LINE__, #cond);               \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

static uint64_t rng_state = 2026;
static uint64_t next_random(void)                   /* splitmix64 */
{
    uint64_t z = (rng_state += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

static void random_string(char *s, size_t len, unsigned alphabet)
{
    for (size_t i = 0; i < len; ++i)
        s[i] = (char)('A' + next_random() % alphabet);
    s[len] = '\0';
}

/* Is s a subsequence of t? */
static int is_subsequence(const char *s, const char *t)
{
    for (; *t != '\0' && *s != '\0'; ++t)
        if (*s == *t)
            ++s;
    return *s == '\0';
}

/* The brute-force answer: try every subset of a's characters (a must be
 * short) and keep the longest that is a subsequence of b. */
static size_t brute_force_length(const char *a, const char *b)
{
    size_t n = strlen(a), best = 0;
    char sub[32];
    for (unsigned long mask = 0; mask < (1UL << n); ++mask) {
        size_t k = 0;
        for (size_t i = 0; i < n; ++i)
            if (mask & (1UL << i))
                sub[k++] = a[i];
        sub[k] = '\0';
        if (k > best && is_subsequence(sub, b))
            best = k;
    }
    return best;
}

/* An answer is right if it is a common subsequence of the right length. */
static int good_answer(const char *a, const char *b, const char *r, size_t want)
{
    return r != NULL && strlen(r) == want && is_subsequence(r, a) && is_subsequence(r, b);
}

static int check_pair(const char *a, const char *b, size_t want)
{
    char *r = lcs_string(a, b);
    int ok = good_answer(a, b, r, want) && lcs_length(a, b) == want && lcs_length(b, a) == want;
    if (!ok)
        printf("  a=\"%s\" b=\"%s\": got \"%s\" and length %zu, wanted length %zu\n", a, b,
               r ? r : "(null)", lcs_length(a, b), want);
    free(r);
    return ok;
}

/* Runs lcs_diff, then rebuilds both inputs from its output: lines marked
 * "  " or "- " must give a, lines marked "  " or "+ " must give b, and the
 * number of changed lines must be na + nb - 2 * LCS. The diff goes to a
 * file in the working directory (not tmpfile(), which fails on Windows
 * without administrator rights). */
static int check_diff(const char *const *a, size_t na, const char *const *b, size_t nb,
                      size_t lcs_lines)
{
    const char *path = "test_lcs_diff.tmp";
    FILE *f = fopen(path, "w");
    if (f == NULL)
        return 0;
    long changes = lcs_diff(a, na, b, nb, f);
    fclose(f);
    if ((f = fopen(path, "r")) == NULL)
        return 0;
    char line[256];
    size_t ia = 0, ib = 0;
    int ok = changes == (long)(na + nb - 2 * lcs_lines);
    while (ok && fgets(line, sizeof line, f) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        const char *text = line + 2;
        if (line[0] == ' ' || line[0] == '-')
            ok = ok && ia < na && strcmp(a[ia++], text) == 0;
        if (line[0] == ' ' || line[0] == '+')
            ok = ok && ib < nb && strcmp(b[ib++], text) == 0;
    }
    fclose(f);
    remove(path);
    return ok && ia == na && ib == nb;
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    /* Edge cases and textbook examples. */
    CHECK(check_pair("", "", 0));
    CHECK(check_pair("", "ABC", 0));
    CHECK(check_pair("ABC", "", 0));
    CHECK(check_pair("ABC", "DEF", 0));
    CHECK(check_pair("ABC", "ABC", 3));
    CHECK(check_pair("ABCBDAB", "BDCABA", 4));
    CHECK(check_pair("AGGTAB", "GXTXAYB", 4));
    char *r = lcs_string("ABC", "DEF");
    CHECK(r != NULL && r[0] == '\0');          /* allocated, so free() is safe */
    free(r);

    /* Every random pair of short strings, against brute force. */
    char a[32], b[32];
    int pairs = 0, pairs_ok = 0;
    for (unsigned alphabet = 2; alphabet <= 6; ++alphabet)
        for (int trial = 0; trial < 2000; ++trial) {
            random_string(a, next_random() % 13, alphabet);
            random_string(b, next_random() % 13, alphabet);
            ++pairs;
            pairs_ok += check_pair(a, b, brute_force_length(a, b));
        }
    printf("short random pairs agreeing with brute force: %d of %d\n", pairs_ok, pairs);

    /* Longer strings: the two-row length must match the full table. */
    static char la[2001], lb[2001];
    int long_ok = 0;
    for (int trial = 0; trial < 20; ++trial) {
        random_string(la, 1 + next_random() % 2000, 4);
        random_string(lb, 1 + next_random() % 2000, 4);
        char *x = lcs_string(la, lb);
        size_t len = lcs_length(la, lb);
        long_ok += good_answer(la, lb, x, len) && len == lcs_length(lb, la);
        free(x);
    }
    printf("long random pairs consistent: %d of 20\n", long_ok);

    /* The diff, on a small text and on random line lists. */
    static const char *const before[] = {"#include <stdio.h>", "int main(void)", "{",
                                         "    printf(\"hello\\n\");", "    return 0;", "}"};
    static const char *const after[] = {"#include <stdio.h>", "#include <stdlib.h>",
                                        "int main(void)", "{", "    puts(\"hello\");",
                                        "    return EXIT_SUCCESS;", "}"};
    CHECK(check_diff(before, 6, after, 7, 4));
    static const char *const words[] = {"alpha", "beta", "gamma", "delta"};
    int diffs_ok = 0;
    for (int trial = 0; trial < 500; ++trial) {
        const char *da[12], *db[12];
        char sa[13], sb[13];
        size_t na = next_random() % 12, nb = next_random() % 12;
        for (size_t i = 0; i < na; ++i) {
            size_t w = next_random() % 4;
            da[i] = words[w];
            sa[i] = (char)('A' + w);
        }
        for (size_t i = 0; i < nb; ++i) {
            size_t w = next_random() % 4;
            db[i] = words[w];
            sb[i] = (char)('A' + w);
        }
        sa[na] = sb[nb] = '\0';
        diffs_ok += check_diff(da, na, db, nb, lcs_length(sa, sb));
    }
    printf("random diffs that rebuild both inputs: %d of 500\n", diffs_ok);
    printf("unit checks failed: %d\n", failures);
    return failures == 0 && pairs_ok == pairs && long_ok == 20 && diffs_ok == 500 ? EXIT_SUCCESS
                                                                                 : EXIT_FAILURE;
}
