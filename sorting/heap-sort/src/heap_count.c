/* heap_count.c - comparisons to build a heap (two ways) and to sort
 * (two variants). Includes heap_sort.c directly so SORT_LESS can count. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static unsigned long long comparisons;
#define SORT_LESS(x, y) (comparisons++, (x) < (y))
#include "heap_sort.c"

static unsigned long long xs = 88172645463325252ULL;
static unsigned long long next_rand(void)
{
    xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
    return xs;
}

static int is_heap(const int a[], size_t n)
{
    for (size_t i = 1; i < n; i++)
        if (a[(i - 1) / 2] < a[i]) return 0;
    return 1;
}
static int is_sorted(const int a[], size_t n)
{
    for (size_t i = 1; i < n; i++)
        if (a[i] < a[i - 1]) return 0;
    return 1;
}

/* shape: 0 random, 1 sorted, 2 reversed, 3 ten distinct values */
static void make_input(int a[], size_t n, int shape)
{
    for (size_t i = 0; i < n; i++)
        a[i] = shape == 1 ? (int)i : shape == 2 ? (int)(n - i)
             : shape == 3 ? (int)(next_rand() % 10) : (int)(next_rand() % 1000000000);
}

int main(void)
{
    static const char *names[] = { "random", "sorted", "reversed", "10 distinct" };
    static const size_t sizes[] = { 10000, 1000000 };
    int *in = malloc(1000000 * sizeof *in), *w = malloc(1000000 * sizeof *w);
    if (!in || !w) { fputs("out of memory\n", stderr); return 1; }

    printf("building a heap: comparisons (and per element)\n");
    printf("%-12s %9s %22s %22s\n", "input", "n", "bottom-up (down)", "insertion (up)");
    for (size_t k = 0; k < 2; k++)
        for (int s = 0; s < 4; s++) {
            size_t n = sizes[k];
            make_input(in, n, s);
            unsigned long long c[2];
            for (int v = 0; v < 2; v++) {
                memcpy(w, in, n * sizeof *w);
                comparisons = 0;
                if (v == 0) make_heap_down(w, n); else make_heap_up(w, n);
                if (!is_heap(w, n)) { puts("NOT A HEAP"); return 1; }
                c[v] = comparisons;
            }
            printf("%-12s %9zu %13llu (%5.2f) %13llu (%5.2f)\n", names[s], n,
                   c[0], (double)c[0] / (double)n, c[1], (double)c[1] / (double)n);
        }

    size_t n = 10000;
    printf("\nsorting n = %zu: comparisons; n log2 n = %.0f\n", n, (double)n * log2((double)n));
    printf("%-12s %14s %14s\n", "input", "heap_sort", "heap_sort_floyd");
    for (int s = 0; s < 4; s++) {
        make_input(in, n, s);
        unsigned long long c[2];
        for (int v = 0; v < 2; v++) {
            memcpy(w, in, n * sizeof *w);
            comparisons = 0;
            if (v == 0) heap_sort(w, n); else heap_sort_floyd(w, n);
            if (!is_sorted(w, n)) { puts("NOT SORTED"); return 1; }
            c[v] = comparisons;
        }
        printf("%-12s %14llu %14llu\n", names[s], c[0], c[1]);
    }
    free(in); free(w);
    return 0;
}
