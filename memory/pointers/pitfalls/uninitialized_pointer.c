/* Deliberately broken: dereferences a pointer that was never set. */
#include <stdio.h>

int main(int argc, char **argv)
{
    int *p;
    (void)argv;
    if (argc > 5)
        p = &argc;
    printf("%d\n", *p);
    return 0;
}
