/* barchart: draws a vertical bar chart from "label value" lines, the way a
   command-line report or a log summary might. It is a pattern program in
   disguise: row by row from the top, a column gets a '#' if its bar is at
   least that tall.

   usage: barchart FILE [HEIGHT]     (HEIGHT defaults to 8 rows) */
#include <stdio.h>
#include <stdlib.h>

#define MAX_BARS 24

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: barchart FILE [HEIGHT]\n");
        return 2;
    }
    int height = argc == 3 ? atoi(argv[2]) : 8;
    if (height < 1 || height > 40) {
        fprintf(stderr, "barchart: HEIGHT must be from 1 to 40\n");
        return 2;
    }
    FILE *in = fopen(argv[1], "r");
    if (in == NULL) {
        fprintf(stderr, "barchart: cannot open %s\n", argv[1]);
        return 2;
    }
    char label[MAX_BARS][4], line[128];
    long value[MAX_BARS], max = 0;
    int n = 0;
    while (n < MAX_BARS && fgets(line, sizeof line, in) != NULL)
        if (line[0] != '#' && sscanf(line, "%3s %ld", label[n], &value[n]) == 2 && value[n] >= 0) {
            if (value[n] > max)
                max = value[n];
            n++;
        }
    fclose(in);
    if (n == 0 || max == 0) {
        fprintf(stderr, "barchart: no values to draw\n");
        return 2;
    }

    int bar[MAX_BARS];
    for (int i = 0; i < n; i++)            /* scale to 0..height, rounding to nearest */
        bar[i] = (int)((value[i] * height + max / 2) / max);

    for (int row = height; row >= 1; row--) {
        int last = -1;
        for (int i = 0; i < n; i++)
            if (bar[i] >= row)
                last = i;                  /* stop after the last bar on this row */
        printf("%6ld |", max * row / height);
        for (int i = 0; i <= last; i++)
            printf(" %s", bar[i] >= row ? "###" : "   ");
        putchar('\n');
    }
    printf("%6s +", "");
    for (int i = 0; i < n; i++)
        printf("----");
    printf("\n%6s  ", "");
    for (int i = 0; i < n; i++)
        printf(" %-3s", label[i]);
    putchar('\n');
    return 0;
}
