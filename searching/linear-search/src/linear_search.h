/* linear_search.h - linear (sequential) search in C: the plain loop, a
 * sentinel version, a generic search with a match function, two
 * self-organizing variants, and binary search for comparison. */
#ifndef LINEAR_SEARCH_H
#define LINEAR_SEARCH_H

#include <stdbool.h>
#include <stddef.h>

/* Index of the first element equal to key, or -1 if there is none. */
ptrdiff_t linear_search(const int *a, size_t n, int key);

/* The same result with one test per element instead of two: the last
 * element is replaced by key while the loop runs, then restored. The
 * array must be writable and not shared with another thread meanwhile. */
ptrdiff_t sentinel_search(int *a, size_t n, int key);

/* The first of n elements of `size` bytes for which match(element, ctx)
 * is true, or NULL. Works for any element type, like lfind(). */
void *search_first(const void *base, size_t n, size_t size,
                   bool (*match)(const void *element, const void *ctx), const void *ctx);

/* Self-organizing search: on a hit, move the element to the front
 * (move_to_front) or one place forward (transpose). Both return the index
 * the element was found at, before it moved, or -1. */
ptrdiff_t search_move_to_front(int *a, size_t n, int key);
ptrdiff_t search_transpose(int *a, size_t n, int key);

/* Index of the first element equal to key in an array sorted in ascending
 * order, or -1. */
ptrdiff_t binary_search(const int *a, size_t n, int key);

#endif
