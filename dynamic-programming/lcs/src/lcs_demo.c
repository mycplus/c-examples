/* lcs_demo.c - the longest common subsequence of a few string pairs, with
 * the table for the first pair.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic lcs_demo.c lcs.c -o lcs_demo */
#include <stdio.h>
#include <stdlib.h>

#include "lcs.h"

static void show(const char *a, const char *b)
{
    char *common = lcs_string(a, b);
    if (common == NULL) {
        fputs("out of memory\n", stderr);
        exit(EXIT_FAILURE);
    }
    printf("%-14s %-14s length %zu: \"%s\"\n", a, b, lcs_length(a, b), common);
    free(common);                       /* always allocated, even when empty */
}

int main(void)
{
    puts("Table of LCS lengths for ABCBDAB and BDCABA:");
    lcs_print_table("ABCBDAB", "BDCABA", stdout);
    puts("");
    show("ABCBDAB", "BDCABA");
    show("BDCABA", "ABCBDAB");
    show("AGGTAB", "GXTXAYB");
    show("dynamic", "programming");
    show("abc", "xyz");
    show("", "anything");
    return 0;
}
