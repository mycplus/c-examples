/* patterns: prints any of the patterns by name.

   usage: patterns NAME ROWS     e.g.  patterns diamond 4
          patterns list          names and sizes of every pattern
   Exit status: 0 on success, 2 for an unknown name or an out-of-range size. */
#include "patterns.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static int usage(void)
{
    fprintf(stderr, "usage: patterns NAME ROWS | patterns list\n");
    return 2;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "list") == 0) {
        for (const pattern *p = patterns; p->name != NULL; p++)
            printf("%-25s 1-%-3d %s\n", p->name, p->max_n, p->about);
        return 0;
    }
    if (argc != 3)
        return usage();

    const pattern *p = pattern_find(argv[1]);
    if (p == NULL) {
        fprintf(stderr, "patterns: no pattern called \"%s\" (try: patterns list)\n", argv[1]);
        return 2;
    }
    char *end;
    errno = 0;
    long n = strtol(argv[2], &end, 10);
    if (end == argv[2] || *end != '\0' || errno == ERANGE || n < 1 || n > p->max_n) {
        fprintf(stderr, "patterns: ROWS for %s must be a whole number from 1 to %d\n", p->name, p->max_n);
        return 2;
    }
    p->draw(stdout, (int)n);
    return 0;
}
