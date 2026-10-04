/* stability.c - which algorithms keep equal keys in input order?
 * Each value packs a key (0-9) and its original position: key * 10000 + pos.
 * SORT_LESS compares only the key, so a stable sort must leave the
 * positions of equal keys in increasing order. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SORT_LESS(x, y) ((x) / 10000 < (y) / 10000)
#include "sorting.c"

/* The row is labelled with the C library whose qsort() it measures. The
 * article's numbers are glibc's; other libraries use other algorithms. */
#ifdef __GLIBC__
#define QSORT_LABEL "qsort() glibc"
#else
#define QSORT_LABEL "qsort() libc"
#endif

#define N 1000

static int cmp_key(const void *pa, const void *pb)
{
    int a = *(const int *)pa / 10000, b = *(const int *)pb / 10000;
    return (a > b) - (a < b);
}

/* 1 if keys are ascending and equal keys keep their input order */
static int is_stable_result(const int a[], size_t n)
{
    for (size_t i = 1; i < n; i++) {
        int k0 = a[i - 1] / 10000, k1 = a[i] / 10000;
        if (k1 < k0) return -1;                       /* not even sorted */
        if (k1 == k0 && a[i] % 10000 < a[i - 1] % 10000) return 0;
    }
    return 1;
}

static void merge_sort_v(int a[], size_t n)
{
    if (merge_sort(a, n) != 0) { fputs("out of memory\n", stderr); exit(1); }
}
static void qsort_v(int a[], size_t n) { qsort(a, n, sizeof a[0], cmp_key); }

int main(void)
{
    static const struct { const char *name; void (*fn)(int *, size_t); } algos[] = {
        { "insertion sort",     insertion_sort  },
        { "selection sort",     selection_sort  },
        { "bubble sort",        bubble_sort     },
        { "shell sort (Knuth)", shell_sort      },
        { "merge sort",         merge_sort_v    },
        { "heap sort",          heap_sort       },
        { "quicksort (middle)", quick_sort      },
        { "quicksort (last)",   quick_sort_last },
        { QSORT_LABEL,          qsort_v         },
    };
    static int input[N], work[N];
    unsigned long long xs = 88172645463325252ULL;

    for (size_t i = 0; i < N; i++) {
        xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
        input[i] = (int)(xs % 10) * 10000 + (int)i;
    }

    printf("%-20s %s\n", "algorithm", "equal keys kept in input order?");
    for (size_t k = 0; k < sizeof algos / sizeof algos[0]; k++) {
        memcpy(work, input, sizeof work);
        algos[k].fn(work, N);
        int r = is_stable_result(work, N);
        if (r < 0) { printf("%s: NOT SORTED\n", algos[k].name); return 1; }
        printf("%-20s %s\n", algos[k].name, r ? "yes" : "no");
    }
    return 0;
}
