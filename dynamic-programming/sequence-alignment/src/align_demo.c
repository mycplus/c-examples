/* align_demo.c - global and local alignments of the article's examples. */
#include <stdio.h>

#include "align.h"
#include "blosum.h"

/* +3 for a match, -3 for a mismatch; rows and columns in the order ACGT */
static const int DNA[4 * 4] = {
     3, -3, -3, -3,
    -3,  3, -3, -3,
    -3, -3,  3, -3,
    -3, -3, -3,  3,
};

static int show(const char *title, const char *s1, const char *s2,
                const Scoring *sc, int local)
{
    Alignment al;
    int err = local ? align_local(s1, s2, sc, &al) : align_global(s1, s2, sc, &al);
    if (err != ALIGN_OK) {
        fprintf(stderr, "%s: %s\n", title, align_error(err));
        return 1;
    }
    printf("%s\n", title);
    alignment_print(&al, sc, stdout);
    printf("score %d", al.score);
    if (local)
        printf(", letters %zu-%zu of the first and %zu-%zu of the second",
               al.start1 + 1, al.end1, al.start2 + 1, al.end2);
    printf("\n\n");
    alignment_free(&al);
    return 0;
}

int main(void)
{
    const Scoring dna_linear = {"ACGT", DNA, 0, 2};            /* every gap position costs 2 */
    const Scoring dna_affine = {"ACGT", DNA, 8, 1};            /* a gap of k costs 8 + k */
    const Scoring blosum50 = {BLOSUM_LETTERS, BLOSUM50, 0, 8}; /* every gap position costs 8 */
    int failed = 0;

    failed |= show("Local, DNA, linear gaps", "TGTTACGG", "GGTTGACTA", &dna_linear, 1);
    failed |= show("Global, DNA, linear gaps", "TGTTACGG", "GGTTGACTA", &dna_linear, 0);
    failed |= show("Global, BLOSUM50, linear gaps", "HEAGAWGHEE", "PAWHEAE", &blosum50, 0);
    failed |= show("Local, BLOSUM50, linear gaps", "HEAGAWGHEE", "PAWHEAE", &blosum50, 1);
    failed |= show("Global, DNA, linear gaps", "GAAAAAGATAGAGC", "GAAAAAGAAAC", &dna_linear, 0);
    failed |= show("Global, DNA, affine gaps", "GAAAAAGATAGAGC", "GAAAAAGAAAC", &dna_affine, 0);
    return failed;
}
