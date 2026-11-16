/* linear_search.c - linear search and its variants. */
#include "linear_search.h"

ptrdiff_t linear_search(const int *a, size_t n, int key)
{
    for (size_t i = 0; i < n; i++)
        if (a[i] == key)
            return (ptrdiff_t)i;
    return -1;
}

ptrdiff_t sentinel_search(int *a, size_t n, int key)
{
    if (n == 0)
        return -1;
    int last = a[n - 1];
    a[n - 1] = key;              /* the loop below is now sure to stop */
    size_t i = 0;
    while (a[i] != key)
        i++;
    a[n - 1] = last;
    if (i < n - 1 || last == key)
        return (ptrdiff_t)i;
    return -1;
}

void *search_first(const void *base, size_t n, size_t size,
                   bool (*match)(const void *element, const void *ctx), const void *ctx)
{
    const unsigned char *p = base;
    for (size_t i = 0; i < n; i++, p += size)
        if (match(p, ctx))
            return (void *)p;
    return NULL;
}

ptrdiff_t search_move_to_front(int *a, size_t n, int key)
{
    ptrdiff_t at = linear_search(a, n, key);
    if (at > 0) {
        for (size_t i = (size_t)at; i > 0; i--)    /* shift the others back */
            a[i] = a[i - 1];
        a[0] = key;
    }
    return at;
}

ptrdiff_t search_transpose(int *a, size_t n, int key)
{
    ptrdiff_t at = linear_search(a, n, key);
    if (at > 0) {
        a[at] = a[at - 1];
        a[at - 1] = key;
    }
    return at;
}

ptrdiff_t binary_search(const int *a, size_t n, int key)
{
    size_t lo = 0, hi = n;    /* the first element >= key is at lo..hi */
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (a[mid] < key)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo < n && a[lo] == key ? (ptrdiff_t)lo : -1;
}
