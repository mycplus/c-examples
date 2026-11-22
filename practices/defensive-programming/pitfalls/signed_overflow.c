/* Adds to an int without a range check. Signed overflow is undefined
 * behavior; UndefinedBehaviorSanitizer reports it at run time. */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int total = argc > 1 ? atoi(argv[1]) : INT_MAX; /* atoi only for brevity */
    total += 1;                                     /* WRONG when total == INT_MAX */
    printf("%d\n", total);
    return 0;
}
