/* Deliberately broken: returns the address of a local variable. */
#include <stdio.h>

static int *make_counter(void)
{
    int count = 42;
    return &count;            /* count stops existing when the function returns */
}

int main(void)
{
    int *p = make_counter();
    printf("%d\n", *p);
    return 0;
}
