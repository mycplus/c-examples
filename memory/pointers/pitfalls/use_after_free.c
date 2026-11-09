/* Deliberately broken: reads memory after freeing it. */
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int *p = malloc(sizeof *p);
    if (p == NULL)
        return 1;
    *p = 7;
    free(p);
    printf("%d\n", *p);       /* p still holds the old address, but the block is gone */
    return 0;
}
