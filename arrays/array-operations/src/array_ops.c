#include <stdio.h>
#include <string.h>

#include "array_ops.h"

void array_print(const int *a, size_t n)
{
    putchar('[');
    for (size_t i = 0; i < n; ++i)
        printf("%s%d", i ? ", " : "", a[i]);
    printf("]\n");
}

int array_find(const int *a, size_t n, int key, size_t *index)
{
    for (size_t i = 0; i < n; ++i) {
        if (a[i] == key) {
            if (index != NULL)
                *index = i;
            return 1;
        }
    }
    return 0;
}

int array_insert(int *a, size_t *n, size_t cap, size_t pos, int value)
{
    if (pos > *n)        /* pos == *n appends; beyond that is a gap */
        return -1;
    if (*n == cap)       /* an array cannot grow */
        return -1;

    /* memmove, not memcpy: source and destination overlap. */
    memmove(&a[pos + 1], &a[pos], (*n - pos) * sizeof a[0]);

    a[pos] = value;
    ++*n;
    return 0;
}

int array_remove(int *a, size_t *n, size_t pos)
{
    if (pos >= *n)
        return -1;

    memmove(&a[pos], &a[pos + 1], (*n - pos - 1) * sizeof a[0]);

    --*n;
    return 0;
}
