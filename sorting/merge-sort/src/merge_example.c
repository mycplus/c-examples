/* merge_example.c - arrays, a linked list, and inversion counting */
#include <stdio.h>
#include <limits.h>
#include "merge_sort.h"

static void print_array(const char *label, const int a[], size_t n)
{
    printf("%-11s", label);
    for (size_t i = 0; i < n; i++)
        printf(" %d", a[i]);
    printf("\n");
}

int main(void)
{
    int a[] = { 29, 10, 14, 37, 13, 5, 41, 22 };
    int b[] = { 29, 10, 14, 37, 13, 5, 41, 22 };
    int c[] = { 29, 10, 14, 37, 13, 5, 41, 22 };
    int edge[] = { 0, INT_MAX, -1, INT_MIN, 0 };
    size_t n = sizeof a / sizeof a[0];

    print_array("before:", a, n);
    if (merge_sort(a, n) != 0 || merge_sort_bottom_up(b, n) != 0) {
        fputs("out of memory\n", stderr);
        return 1;
    }
    print_array("top-down:", a, n);
    print_array("bottom-up:", b, n);

    struct node nodes[8], *head = NULL;      /* build a list 29 -> 10 -> ... */
    for (size_t i = n; i-- > 0; ) {
        nodes[i].value = c[i];
        nodes[i].next = head;
        head = &nodes[i];
    }
    head = list_merge_sort(head);
    printf("%-11s", "list:");
    for (struct node *p = head; p != NULL; p = p->next)
        printf(" %d", p->value);
    printf("\n");

    printf("inversions: %llu\n", count_inversions(c, n));
    if (merge_sort(edge, 5) != 0)
        return 1;
    print_array("limits:", edge, 5);
    return 0;
}
