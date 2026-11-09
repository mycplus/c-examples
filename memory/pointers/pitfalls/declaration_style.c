/* The * binds to the name, not the type: only a is a pointer. */
#include <stdio.h>

int main(void)
{
    int* a, b;
    printf("sizeof a = %zu, sizeof b = %zu\n", sizeof a, sizeof b);
    return 0;
}
