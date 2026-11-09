/* Deliberately broken: frees the same block twice. */
#include <stdlib.h>

int main(void)
{
    int *p = malloc(sizeof *p);
    free(p);
    free(p);                  /* undefined behavior; the allocator's bookkeeping is corrupted */
    return 0;
}
