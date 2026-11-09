/* Deliberately broken: a void * cannot be dereferenced, and arithmetic on it is not standard C. */
#include <stdio.h>

int main(void)
{
    int x = 5;
    void *v = &x;
    printf("%d\n", *v);
    v = v + 1;
    return 0;
}
