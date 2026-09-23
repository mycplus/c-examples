/* bubble_count.c - count comparisons and swaps for three bubble sort variants */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { unsigned long long cmp, swp; } Counts;

static void swap_int(int *x, int *y) { int t = *x; *x = *y; *y = t; }

/* 1. Textbook: n-1 passes, each one element shorter, no early exit. */
static Counts plain(int a[], size_t n)
{
    Counts c = {0, 0};
    for (size_t i = 1; i < n; i++)
        for (size_t j = 1; j <= n - i; j++) {
            c.cmp++;
            if (a[j] < a[j - 1]) { swap_int(&a[j], &a[j - 1]); c.swp++; }
        }
    return c;
}

/* 2. Swapped flag: stop after the first pass that makes no swap. */
static Counts flag(int a[], size_t n)
{
    Counts c = {0, 0};
    for (size_t i = 1; i < n; i++) {
        int swapped = 0;
        for (size_t j = 1; j <= n - i; j++) {
            c.cmp++;
            if (a[j] < a[j - 1]) { swap_int(&a[j], &a[j - 1]); c.swp++; swapped = 1; }
        }
        if (!swapped)
            break;
    }
    return c;
}

/* 3. Last-swap boundary: the next pass stops where this one last swapped. */
static Counts boundary(int a[], size_t n)
{
    Counts c = {0, 0};
    size_t bound = n;
    while (bound > 1) {
        size_t last_swap = 0;
        for (size_t j = 1; j < bound; j++) {
            c.cmp++;
            if (a[j] < a[j - 1]) { swap_int(&a[j], &a[j - 1]); c.swp++; last_swap = j; }
        }
        bound = last_swap;
    }
    return c;
}

/* For contrast: insertion sort, counting comparisons only. */
static Counts insertion(int a[], size_t n)
{
    Counts c = {0, 0};
    for (size_t i = 1; i < n; i++) {
        int key = a[i];
        size_t j = i;
        while (j > 0) {
            c.cmp++;
            if (a[j - 1] <= key)
                break;
            a[j] = a[j - 1];
            j--;
        }
        a[j] = key;
    }
    return c;
}

/* Inversions (pairs i < j with a[i] > a[j]), counted independently by merge sort. */
static unsigned long long inversions(int a[], int tmp[], size_t n)
{
    if (n < 2)
        return 0;
    size_t mid = n / 2;
    unsigned long long inv = inversions(a, tmp, mid) + inversions(a + mid, tmp, n - mid);
    size_t i = 0, j = mid, k = 0;
    while (i < mid && j < n) {
        if (a[j] < a[i]) { tmp[k++] = a[j++]; inv += mid - i; }
        else             { tmp[k++] = a[i++]; }
    }
    while (i < mid) tmp[k++] = a[i++];
    while (j < n)   tmp[k++] = a[j++];
    memcpy(a, tmp, n * sizeof *a);
    return inv;
}

static unsigned int rng = 2463534242u;          /* xorshift32: same on every platform */
static unsigned int next_rand(void)
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return rng;
}

enum { N = 1000 };

static void fill(int a[], const char *shape)
{
    for (int i = 0; i < N; i++) a[i] = i;                         /* sorted */
    if (strcmp(shape, "reversed") == 0)
        for (int i = 0; i < N; i++) a[i] = N - 1 - i;
    else if (strcmp(shape, "random") == 0)
        for (int i = 0; i < N; i++) a[i] = (int)(next_rand() % 1000000);
    else if (strcmp(shape, "smallest-last") == 0) {               /* 1..999, 0 */
        for (int i = 0; i < N - 1; i++) a[i] = i + 1;
        a[N - 1] = 0;
    } else if (strcmp(shape, "largest-first") == 0) {             /* 999, 0..998 */
        a[0] = N - 1;
        for (int i = 1; i < N; i++) a[i] = i - 1;
    }
}

int main(void)
{
    static int base[N], work[N], tmp[N];
    const char *shapes[] = { "sorted", "random", "reversed", "smallest-last", "largest-first" };
    Counts (*sorts[])(int[], size_t) = { plain, flag, boundary, insertion };

    printf("comparisons, n = %d\n%-14s %9s %9s %9s %10s %8s\n", N, "input",
           "plain", "flag", "boundary", "insertion", "swaps");

    for (size_t s = 0; s < sizeof shapes / sizeof shapes[0]; s++) {
        fill(base, shapes[s]);
        unsigned long long cmp[4], swp[4];

        for (int v = 0; v < 4; v++) {
            memcpy(work, base, sizeof base);
            Counts c = sorts[v](work, N);
            cmp[v] = c.cmp;
            swp[v] = c.swp;
            for (int i = 1; i < N; i++)
                if (work[i] < work[i - 1]) { puts("NOT SORTED"); return 1; }
        }
        memcpy(work, base, sizeof base);
        unsigned long long inv = inversions(work, tmp, N);

        printf("%-14s %9llu %9llu %9llu %10llu %8llu\n", shapes[s],
               cmp[0], cmp[1], cmp[2], cmp[3], swp[0]);
        if (swp[0] != swp[1] || swp[1] != swp[2] || swp[2] != inv) {
            puts("swap counts disagree");
            return 1;
        }
    }
    puts("every variant's swap count equals the independently counted inversions");
    return 0;
}
