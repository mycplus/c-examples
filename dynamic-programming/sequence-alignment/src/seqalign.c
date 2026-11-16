/* seqalign.c - align the first sequence of two FASTA files.
 *
 *   seqalign [-local] [-matrix BLOSUM62|BLOSUM50|DNA] [-open N] [-extend N]
 *            [-score] FILE1 FILE2
 *
 * A gap of length k costs open + k * extend (defaults 11 and 1). DNA scores
 * +3 for a match and -3 for a mismatch. With -score only the score is
 * printed, computed in linear memory. Exit status: 0 on success, 2 on error. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "align.h"
#include "blosum.h"

static const int DNA[4 * 4] = {
     3, -3, -3, -3,
    -3,  3, -3, -3,
    -3, -3,  3, -3,
    -3, -3, -3,  3,
};

/* Reads the first record of a FASTA file (or a file of bare letters):
 * header lines start with '>', and whitespace is skipped. */
static char *read_fasta(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror(path);
        return NULL;
    }
    size_t len = 0, cap = 256;
    char *seq = malloc(cap);
    int c, records = 0, at_line_start = 1;
    while (seq != NULL && (c = fgetc(f)) != EOF) {
        if (at_line_start && c == '>') {
            if (++records > 1)
                break;                         /* only the first record */
            while ((c = fgetc(f)) != EOF && c != '\n')
                ;                              /* skip the header */
            continue;
        }
        at_line_start = c == '\n';
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            continue;
        if (len + 1 == cap) {
            char *bigger = realloc(seq, cap * 2);
            if (bigger == NULL) {
                free(seq);
                seq = NULL;
                break;
            }
            seq = bigger;
            cap *= 2;
        }
        seq[len++] = (char)c;
    }
    fclose(f);
    if (seq == NULL)
        fprintf(stderr, "%s: out of memory\n", path);
    else
        seq[len] = '\0';
    return seq;
}

static int usage(void)
{
    fputs("usage: seqalign [-local] [-matrix BLOSUM62|BLOSUM50|DNA] [-open N]"
          " [-extend N] [-score] FILE1 FILE2\n", stderr);
    return 2;
}

int main(int argc, char **argv)
{
    Scoring sc = {BLOSUM_LETTERS, BLOSUM62, 11, 1};
    int local = 0, score_only = 0, i;
    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        if (strcmp(argv[i], "-local") == 0)
            local = 1;
        else if (strcmp(argv[i], "-score") == 0)
            score_only = 1;
        else if (i + 1 < argc && strcmp(argv[i], "-open") == 0)
            sc.gap_open = atoi(argv[++i]);
        else if (i + 1 < argc && strcmp(argv[i], "-extend") == 0)
            sc.gap_extend = atoi(argv[++i]);
        else if (i + 1 < argc && strcmp(argv[i], "-matrix") == 0) {
            const char *name = argv[++i];
            if (strcmp(name, "BLOSUM62") == 0)
                sc.table = BLOSUM62;
            else if (strcmp(name, "BLOSUM50") == 0)
                sc.table = BLOSUM50;
            else if (strcmp(name, "DNA") == 0)
                sc.letters = "ACGT", sc.table = DNA;
            else
                return usage();
        } else
            return usage();
    }
    if (argc - i != 2)
        return usage();

    char *s1 = read_fasta(argv[i]), *s2 = s1 ? read_fasta(argv[i + 1]) : NULL;
    int err = ALIGN_NO_MEMORY;
    if (s1 != NULL && s2 != NULL) {
        if (score_only) {
            int score = 0;
            err = align_score(s1, s2, &sc, local, &score);
            if (err == ALIGN_OK)
                printf("%d\n", score);
        } else {
            Alignment al;
            err = local ? align_local(s1, s2, &sc, &al) : align_global(s1, s2, &sc, &al);
            if (err == ALIGN_OK) {
                alignment_print(&al, &sc, stdout);
                printf("score %d, letters %zu-%zu of the first and %zu-%zu of the second\n",
                       al.score, al.start1 + 1, al.end1, al.start2 + 1, al.end2);
                alignment_free(&al);
            }
        }
        if (err != ALIGN_OK)
            fprintf(stderr, "seqalign: %s\n", align_error(err));
    }
    free(s1);
    free(s2);
    return err == ALIGN_OK ? 0 : 2;
}
