/* heap_sort.h - heap sort for int arrays, and the two ways to build a heap */
#ifndef HEAP_SORT_H
#define HEAP_SORT_H

#include <stddef.h>

void heap_sort(int a[], size_t n);         /* sift-down, bottom-up build  */
void heap_sort_floyd(int a[], size_t n);   /* Floyd's leaf-first variant  */

void make_heap_down(int a[], size_t n);    /* bottom-up build: O(n)       */
void make_heap_up(int a[], size_t n);      /* insert one at a time: O(n log n) */

#endif
