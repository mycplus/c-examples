/* test_merge_sort.c - both array sorts against qsort() and the list sort
 * against the array result, on 20,000 random inputs of 0 to 64 elements;
 * count_inversions() against a brute-force count. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "merge_sort.h"

static int cmp_int(const void *pa, const void *pb)
{
    int a = *(const int *)pa, b = *(const int *)pb;
    return (a > b) - (a < b);
}

int main(void)
{
    unsigned long long xs = 0x9E3779B97F4A7C15ULL;
    int in[64], ref[64], out[64], failures = 0;
    struct node nodes[64];

    for (int t = 0; t < 20000; t++) {
        xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
        size_t n = (size_t)(xs % 65);
        int range = (t % 3 == 0) ? 4 : 1000;
        for (size_t i = 0; i < n; i++) {
            xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
            in[i] = (xs % 17 == 0) ? INT_MIN : (xs % 17 == 1) ? INT_MAX
                  : (int)(xs % (unsigned)range) - range / 2;
        }
        memcpy(ref, in, n * sizeof in[0]);
        qsort(ref, n, sizeof ref[0], cmp_int);

        memcpy(out, in, n * sizeof in[0]);
        if (merge_sort(n ? out : NULL, n) != 0 || (n && memcmp(out, ref, n * sizeof out[0]) != 0)) {
            printf("FAIL merge_sort case %d\n", t); failures++;
        }
        memcpy(out, in, n * sizeof in[0]);
        if (merge_sort_bottom_up(n ? out : NULL, n) != 0 || (n && memcmp(out, ref, n * sizeof out[0]) != 0)) {
            printf("FAIL merge_sort_bottom_up case %d\n", t); failures++;
        }

        struct node *head = NULL;
        for (size_t i = n; i-- > 0; ) { nodes[i].value = in[i]; nodes[i].next = head; head = &nodes[i]; }
        head = list_merge_sort(head);
        size_t k = 0;
        for (struct node *p = head; p != NULL; p = p->next, k++)
            if (k >= n || p->value != ref[k]) { k = n + 1; break; }
        if (k != n) { printf("FAIL list_merge_sort case %d\n", t); failures++; }

        unsigned long long brute = 0;
        for (size_t i = 0; i < n; i++)
            for (size_t j = i + 1; j < n; j++)
                if (in[j] < in[i]) brute++;
        memcpy(out, in, n * sizeof in[0]);
        if (count_inversions(n ? out : NULL, n) != brute) { printf("FAIL count_inversions case %d\n", t); failures++; }
    }
    printf("%s: 4 functions x 20000 inputs of 0-64 elements\n", failures ? "FAILED" : "passed");
    return failures != 0;
}
