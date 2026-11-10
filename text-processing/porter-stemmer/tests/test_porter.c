/* test_porter.c - the measure examples from the paper, edge cases, and every
 * word of the test vocabulary in both modes.
 * Usage: test_porter VOC PAPER_OUTPUT STANDARD_OUTPUT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "porter.h"

static int failures;

static void check(int ok, const char *what)
{
    if (!ok) {
        printf("FAILED: %s\n", what);
        ++failures;
    }
}

static void expect_stem(const char *in, enum porter_mode mode, const char *out)
{
    char buf[64];
    strcpy(buf, in);
    size_t n = porter_stem(buf, mode);
    if (strcmp(buf, out) != 0 || n != strlen(out)) {
        printf("FAILED: %s -> \"%s\", expected \"%s\" (%s)\n", in, buf, out,
               mode == PORTER_PAPER ? "paper" : "standard");
        ++failures;
    }
}

/* Compares every line of voc stemmed in `mode` with the expected file. */
static unsigned long run_vocabulary(const char *voc, const char *expected, enum porter_mode mode,
                                    unsigned long *total)
{
    FILE *v = fopen(voc, "r"), *e = fopen(expected, "r");
    if (v == NULL || e == NULL) {
        perror(v == NULL ? voc : expected);
        exit(EXIT_FAILURE);
    }
    char word[256], want[256];
    unsigned long wrong = 0;
    *total = 0;
    while (fgets(word, sizeof word, v) != NULL) {
        if (fgets(want, sizeof want, e) == NULL) {
            puts("FAILED: expected file is shorter than the vocabulary");
            exit(EXIT_FAILURE);
        }
        word[strcspn(word, "\r\n")] = '\0';
        want[strcspn(want, "\r\n")] = '\0';
        char stem[256];
        strcpy(stem, word);
        porter_stem(stem, mode);
        if (strcmp(stem, want) != 0) {
            if (++wrong <= 5)
                printf("  %s -> %s, expected %s\n", word, stem, want);
        }
        ++*total;
    }
    fclose(v);
    fclose(e);
    return wrong;
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fputs("usage: test_porter VOC PAPER_OUTPUT STANDARD_OUTPUT\n", stderr);
        return 2;
    }

    /* The measure m, with the paper's own examples. */
    static const struct { const char *w; unsigned m; } measures[] = {
        {"tr", 0}, {"ee", 0}, {"tree", 0}, {"y", 0}, {"by", 0},
        {"trouble", 1}, {"oats", 1}, {"trees", 1}, {"ivy", 1},
        {"troubles", 2}, {"private", 2}, {"oaten", 2}, {"orrery", 2},
    };
    for (size_t i = 0; i < sizeof measures / sizeof measures[0]; ++i)
        check(porter_measure(measures[i].w, strlen(measures[i].w)) == measures[i].m,
              measures[i].w);

    /* Edge cases. */
    expect_stem("", PORTER_STANDARD, "");
    expect_stem("", PORTER_PAPER, "");
    expect_stem("s", PORTER_STANDARD, "s");
    expect_stem("s", PORTER_PAPER, "");          /* the paper strips a lone s */
    expect_stem("as", PORTER_STANDARD, "as");
    expect_stem("as", PORTER_PAPER, "a");
    expect_stem("possibly", PORTER_STANDARD, "possibl");
    expect_stem("possibly", PORTER_PAPER, "possibli");
    expect_stem("archaeology", PORTER_STANDARD, "archaeolog");
    expect_stem("archaeology", PORTER_PAPER, "archaeologi");
    expect_stem("syzygy", PORTER_STANDARD, "syzygi"); /* y after a consonant is a vowel */
    expect_stem("ied", PORTER_STANDARD, "i");      /* step 1 leaves one letter; */
    expect_stem("ied", PORTER_PAPER, "i");         /* later steps must not read b[-1] */

    unsigned long total;
    unsigned long wrong_paper = run_vocabulary(argv[1], argv[2], PORTER_PAPER, &total);
    unsigned long wrong_standard = run_vocabulary(argv[1], argv[3], PORTER_STANDARD, &total);
    printf("%lu words: paper mode %lu wrong, standard mode %lu wrong\n", total, wrong_paper,
           wrong_standard);
    printf("unit checks failed: %d\n", failures);
    return failures == 0 && wrong_paper == 0 && wrong_standard == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
