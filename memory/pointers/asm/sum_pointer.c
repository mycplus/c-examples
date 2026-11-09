#include <stddef.h>

int sum(const int *a, size_t n)
{
    int total = 0;
    for (const int *p = a, *end = a + n; p != end; p++)
        total += *p;
    return total;
}
