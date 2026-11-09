/* Deliberately broken: sizeof on an array parameter measures a pointer. */
#include <stdio.h>

static size_t count_items(int a[10])
{
    return sizeof a / sizeof a[0];
}

int main(void)
{
    int values[10] = {0};
    printf("real length: %zu, computed inside the function: %zu\n",
           sizeof values / sizeof values[0], count_items(values));
    return 0;
}
