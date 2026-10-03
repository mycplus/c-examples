/* merge_sort.h - merge sort for int arrays and linked lists, and
 * inversion counting */
#ifndef MERGE_SORT_H
#define MERGE_SORT_H

#include <stddef.h>

int merge_sort(int a[], size_t n);            /* top-down; -1 if malloc fails */
int merge_sort_bottom_up(int a[], size_t n);  /* iterative; -1 if malloc fails */

struct node {
    int value;
    struct node *next;
};
struct node *list_merge_sort(struct node *head);   /* no buffer; O(log n) stack */

/* Number of pairs i < j with a[i] > a[j]. Sorts a as a side effect.
 * Returns -1 (as unsigned long long: ULLONG_MAX) if malloc fails. */
unsigned long long count_inversions(int a[], size_t n);

#endif
