/* Deliberately broken: the only pointer to the block is overwritten. */
#include <stdlib.h>

int main(void)
{
    int *p = malloc(100 * sizeof *p);
    p = NULL;                 /* the 400 bytes are now unreachable and never freed */
    return p != NULL;
}
