/* stem_demo.c - stems a few groups of related words, then shows the three
 * words where Martin Porter's own implementation and the 1980 paper differ.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic stem_demo.c porter.c -o stem_demo */
#include <stdio.h>
#include <string.h>

#include "porter.h"

static void show(const char *const words[], size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        char buf[64];
        strcpy(buf, words[i]);
        porter_stem(buf, PORTER_STANDARD);
        printf("  %-16s -> %s\n", words[i], buf);
    }
}

int main(void)
{
    static const char *const connect[] = {"connect", "connected", "connecting",
                                          "connection", "connections"};
    static const char *const others[] = {"caresses", "ponies", "hopping", "filing",
                                         "happy", "relational", "generalizations",
                                         "probate", "controlling", "university",
                                         "universe"};
    puts("One stem for related words:");
    show(connect, sizeof connect / sizeof connect[0]);
    puts("Stems are keys, not dictionary words:");
    show(others, sizeof others / sizeof others[0]);

    puts("Where Porter's code and the 1980 paper differ:");
    static const char *const departures[] = {"as", "possibly", "archaeology"};
    for (size_t i = 0; i < 3; ++i) {
        char standard[64], paper[64];
        strcpy(standard, departures[i]);
        strcpy(paper, departures[i]);
        porter_stem(standard, PORTER_STANDARD);
        porter_stem(paper, PORTER_PAPER);
        printf("  %-16s standard: %-12s paper: %s\n", departures[i], standard, paper);
    }
    return 0;
}
