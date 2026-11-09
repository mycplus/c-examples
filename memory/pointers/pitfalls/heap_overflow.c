/* Deliberately broken: the loop runs one element past the end of the block. */
#include <stdlib.h>

int main(void)
{
    int *a = malloc(4 * sizeof *a);
    if (a == NULL)
        return 1;
    for (int i = 0; i <= 4; i++)      /* should be i < 4 */
        a[i] = i;
    free(a);
    return 0;
}
