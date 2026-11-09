/* Deliberately broken: the function changes its own copy of the pointer. */
#include <stdio.h>
#include <stdlib.h>

static void make_buffer(int *buf)
{
    buf = malloc(10 * sizeof *buf);   /* only the local copy of buf changes */
    (void)buf;
}

int main(void)
{
    int *data = NULL;
    make_buffer(data);
    printf("data is %s\n", data == NULL ? "still NULL" : "allocated");
    return 0;
}
