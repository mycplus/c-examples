/* merge_sort.c - merge sort in C11.
 * SORT_LESS(x, y) means "x sorts before y". Test programs redefine it
 * before including this file, to count comparisons. MERGE_SKIP_SORTED
 * (default 1) skips a merge when the two halves are already in order. */
#include <stdlib.h>
#include <string.h>
#include "merge_sort.h"

#ifndef SORT_LESS
#define SORT_LESS(x, y) ((x) < (y))
#endif
#ifndef MERGE_SKIP_SORTED
#define MERGE_SKIP_SORTED 1
#endif

/* Merges the sorted runs a[lo..mid) and a[mid..hi) through buf. On a tie
 * the left element goes first, which is what makes the sort stable. */
static void merge(int a[], int buf[], size_t lo, size_t mid, size_t hi)
{
    size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi)
        buf[k++] = SORT_LESS(a[j], a[i]) ? a[j++] : a[i++];
    while (i < mid) buf[k++] = a[i++];
    while (j < hi)  buf[k++] = a[j++];
    memcpy(a + lo, buf + lo, (hi - lo) * sizeof a[0]);
}

/* Sorts the half-open range a[lo..hi). */
static void merge_sort_rec(int a[], int buf[], size_t lo, size_t hi)
{
    if (hi - lo < 2)                         /* 0 or 1 element: sorted */
        return;
    size_t mid = lo + (hi - lo) / 2;         /* cannot overflow */
    merge_sort_rec(a, buf, lo, mid);
    merge_sort_rec(a, buf, mid, hi);
    if (MERGE_SKIP_SORTED && !SORT_LESS(a[mid], a[mid - 1]))
        return;                              /* halves already in order */
    merge(a, buf, lo, mid, hi);
}

int merge_sort(int a[], size_t n)
{
    if (n < 2)
        return 0;
    int *buf = malloc(n * sizeof *buf);      /* one buffer for every merge */
    if (buf == NULL)
        return -1;
    merge_sort_rec(a, buf, 0, n);
    free(buf);
    return 0;
}

/* Bottom-up: merge runs of width 1, 2, 4, ... No recursion. */
int merge_sort_bottom_up(int a[], size_t n)
{
    if (n < 2)
        return 0;
    int *buf = malloc(n * sizeof *buf);
    if (buf == NULL)
        return -1;
    for (size_t width = 1; width < n; width *= 2)
        for (size_t lo = 0; lo < n - width; lo += 2 * width) {
            size_t mid = lo + width;
            size_t hi = (n - mid > width) ? mid + width : n;
            merge(a, buf, lo, mid, hi);
        }
    free(buf);
    return 0;
}

/* Linked list: split with slow/fast pointers, merge by relinking nodes. */
static struct node *list_merge(struct node *x, struct node *y)
{
    struct node head = { 0, NULL }, *tail = &head;
    while (x != NULL && y != NULL) {
        if (SORT_LESS(y->value, x->value)) { tail->next = y; y = y->next; }
        else                               { tail->next = x; x = x->next; }
        tail = tail->next;
    }
    tail->next = (x != NULL) ? x : y;
    return head.next;
}

struct node *list_merge_sort(struct node *head)
{
    if (head == NULL || head->next == NULL)
        return head;
    struct node *slow = head, *fast = head->next;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }
    struct node *second = slow->next;        /* cut the list in two */
    slow->next = NULL;
    return list_merge(list_merge_sort(head), list_merge_sort(second));
}

/* Inversion counting: when an element is taken from the right run, it is
 * smaller than every element still waiting in the left run. */
static unsigned long long count_rec(int a[], int buf[], size_t lo, size_t hi)
{
    if (hi - lo < 2)
        return 0;
    size_t mid = lo + (hi - lo) / 2;
    unsigned long long inv = count_rec(a, buf, lo, mid) + count_rec(a, buf, mid, hi);
    size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        if (SORT_LESS(a[j], a[i])) {
            inv += mid - i;                  /* a[j] < a[i..mid) */
            buf[k++] = a[j++];
        } else {
            buf[k++] = a[i++];
        }
    }
    while (i < mid) buf[k++] = a[i++];
    while (j < hi)  buf[k++] = a[j++];
    memcpy(a + lo, buf + lo, (hi - lo) * sizeof a[0]);
    return inv;
}

unsigned long long count_inversions(int a[], size_t n)
{
    if (n < 2)
        return 0;
    int *buf = malloc(n * sizeof *buf);
    if (buf == NULL)
        return (unsigned long long)-1;
    unsigned long long inv = count_rec(a, buf, 0, n);
    free(buf);
    return inv;
}
