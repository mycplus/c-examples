/* array_ops.h - operations on a fixed-capacity array of int.
 *
 * A C array does not know its own length, so every function takes the
 * element count explicitly. Functions that change the count take a
 * pointer to it and the capacity, because an array cannot grow.
 */
#ifndef MYCPLUS_ARRAY_OPS_H
#define MYCPLUS_ARRAY_OPS_H

#include <stddef.h>

/* Write n elements to stdout as "[a, b, c]". */
void array_print(const int *a, size_t n);

/* Linear search for key in the first n elements.
   Returns 1 and stores the index in *index when found, 0 otherwise.
   The index is reported through a parameter because size_t is unsigned:
   there is no negative value available to mean "not found". */
int array_find(const int *a, size_t n, int key, size_t *index);

/* Insert value at position pos, shifting later elements right.
   pos may equal *n, which appends. Returns 0 on success, or -1 when
   pos > *n or the array is already at capacity. */
int array_insert(int *a, size_t *n, size_t cap, size_t pos, int value);

/* Remove the element at pos, shifting later elements left.
   Returns 0 on success, or -1 when pos >= *n. */
int array_remove(int *a, size_t *n, size_t pos);

#endif /* MYCPLUS_ARRAY_OPS_H */
