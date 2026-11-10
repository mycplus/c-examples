/* Deliberately broken: == compares the arrays' addresses, not their contents. */
#include <stdio.h>

int main(void)
{
    int a[3] = {1, 2, 3};
    int b[3] = {1, 2, 3};
    printf("a == b: %s\n", a == b ? "equal" : "not equal");
    return 0;
}
