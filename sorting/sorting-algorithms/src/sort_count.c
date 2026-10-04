/* sort_count.c - comparisons made by each algorithm on six inputs of
 * 10,000 values. The first five are generated exactly as in the insertion
 * sort and merge sort articles, so those rows match their tables.
 * Build: gcc -std=c11 -Wall -Wextra -O2 sort_count.c -o sort_count
 * (sorting.c is included directly so SORT_LESS can count.) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long long comparisons;
#define SORT_LESS(x, y) (comparisons++, (x) < (y))
#include "sorting.c"

/* The row is labelled with the C library whose qsort() it measures. The
 * article's numbers are glibc's; other libraries use other algorithms. */
#ifdef __GLIBC__
#define QSORT_LABEL "qsort() glibc"
#else
#define QSORT_LABEL "qsort() libc"
#endif

#define N 10000

static unsigned long long xs = 88172645463325252ULL;   /* fixed seed */
static unsigned long long next_rand(void)
{
    xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
    return xs;
}

static int cmp_int(const void *pa, const void *pb)
{
    int a = *(const int *)pa, b = *(const int *)pb;
    comparisons++;
    return (a > b) - (a < b);
}

/* shapes 0-4 in the order of the insertion and merge sort articles */
static void make_input(int a[], int shape)
{
    for (size_t i = 0; i < N; i++)
        a[i] = (int)(next_rand() % 1000000);
    if (shape == 5) {                                   /* ten distinct values */
        for (size_t i = 0; i < N; i++) a[i] %= 10;
        return;
    }
    if (shape != 2)
        qsort(a, N, sizeof a[0], cmp_int);
    if (shape == 1)                                     /* reversed */
        for (size_t i = 0; i < N / 2; i++) swap_int(&a[i], &a[N - 1 - i]);
    if (shape == 3)                                     /* shuffle inside blocks of 8 */
        for (size_t b = 0; b < N; b += 8)
            for (size_t k = 7; k > 0; k--)
                swap_int(&a[b + k], &a[b + (size_t)(next_rand() % (k + 1))]);
    if (shape == 4)                                     /* 100 far swaps */
        for (int k = 0; k < 100; k++) {
            size_t i = next_rand() % N, j = next_rand() % N;
            swap_int(&a[i], &a[j]);
        }
}

typedef struct { const char *name; void (*fn)(int *, size_t); } algo;

static void merge_sort_v(int a[], size_t n)
{
    if (merge_sort(a, n) != 0) { fputs("out of memory\n", stderr); exit(1); }
}
static void qsort_v(int a[], size_t n) { qsort(a, n, sizeof a[0], cmp_int); }

int main(void)
{
    static const char *shapes[] = { "sorted", "reversed", "random",
                                    "local", "far swaps", "10 distinct" };
    static const algo algos[] = {
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
    static int input[6][N], work[N], ref[N];

    for (int s = 0; s < 6; s++)
        make_input(input[s], s);

    printf("comparisons, n = %d, MERGE_SKIP_SORTED = %d\n%-19s", N, MERGE_SKIP_SORTED, "algorithm");
    for (int s = 0; s < 6; s++)
        printf("%12s", shapes[s]);
    printf("\n");

    for (size_t k = 0; k < sizeof algos / sizeof algos[0]; k++) {
        printf("%-19s", algos[k].name);
        for (int s = 0; s < 6; s++) {
            memcpy(ref, input[s], sizeof ref);
            qsort(ref, N, sizeof ref[0], cmp_int);
            memcpy(work, input[s], sizeof work);
            comparisons = 0;
            algos[k].fn(work, N);
            if (memcmp(work, ref, sizeof work) != 0) {
                printf("\nWRONG RESULT: %s on %s\n", algos[k].name, shapes[s]);
                return 1;
            }
            printf("%12llu", comparisons);
        }
        printf("\n");
    }
    printf("every result matched a qsort() reference\n");
    return 0;
}
