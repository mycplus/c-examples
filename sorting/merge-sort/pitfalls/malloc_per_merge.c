/* malloc_per_merge.c - allocating a temporary array inside every merge.
 * It sorts correctly; count the allocations and time it on 1,000,000 ints.
 * POSIX (clock_gettime). */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N 1000000

static unsigned long long allocations;

static void merge_alloc(int a[], size_t lo, size_t mid, size_t hi)
{
    int *tmp = malloc((hi - lo) * sizeof *tmp);        /* one per merge */
    if (tmp == NULL) { fputs("out of memory\n", stderr); exit(1); }
    allocations++;
    size_t i = lo, j = mid, k = 0;
    while (i < mid && j < hi)
        tmp[k++] = (a[j] < a[i]) ? a[j++] : a[i++];
    while (i < mid) tmp[k++] = a[i++];
    while (j < hi)  tmp[k++] = a[j++];
    memcpy(a + lo, tmp, (hi - lo) * sizeof *tmp);
    free(tmp);
}

static void sort_alloc(int a[], size_t lo, size_t hi)
{
    if (hi - lo < 2)
        return;
    size_t mid = lo + (hi - lo) / 2;
    sort_alloc(a, lo, mid);
    sort_alloc(a, mid, hi);
    merge_alloc(a, lo, mid, hi);
}

static void merge_once(int a[], int buf[], size_t lo, size_t mid, size_t hi)
{
    size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi)
        buf[k++] = (a[j] < a[i]) ? a[j++] : a[i++];
    while (i < mid) buf[k++] = a[i++];
    while (j < hi)  buf[k++] = a[j++];
    memcpy(a + lo, buf + lo, (hi - lo) * sizeof *buf);
}

static void sort_once(int a[], int buf[], size_t lo, size_t hi)   /* same, one buffer */
{
    if (hi - lo < 2)
        return;
    size_t mid = lo + (hi - lo) / 2;
    sort_once(a, buf, lo, mid);
    sort_once(a, buf, mid, hi);
    merge_once(a, buf, lo, mid, hi);
}

static double now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec * 1e3 + (double)t.tv_nsec / 1e6;
}

static int cmp_double(const void *pa, const void *pb)
{
    double a = *(const double *)pa, b = *(const double *)pb;
    return (a > b) - (a < b);
}

int main(void)
{
    int *in = malloc(N * sizeof *in), *a = malloc(N * sizeof *a), *b = malloc(N * sizeof *b);
    int *buf = malloc(N * sizeof *buf);
    if (!in || !a || !b || !buf) { fputs("out of memory\n", stderr); return 1; }
    unsigned long long xs = 88172645463325252ULL;
    for (size_t i = 0; i < N; i++) {
        xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
        in[i] = (int)(xs % 1000000000);
    }
    double ta[5], tb[5];
    for (int r = 0; r < 5; r++) {                 /* interleaved, median of 5 */
        memcpy(a, in, N * sizeof *a);
        memcpy(b, in, N * sizeof *b);
        allocations = 0;
        double t0 = now_ms();
        sort_alloc(a, 0, N);
        double t1 = now_ms();
        sort_once(b, buf, 0, N);
        double t2 = now_ms();
        ta[r] = t1 - t0;
        tb[r] = t2 - t1;
        if (memcmp(a, b, N * sizeof *a) != 0) { puts("results DIFFER"); return 1; }
    }
    qsort(ta, 5, sizeof ta[0], cmp_double);
    qsort(tb, 5, sizeof tb[0], cmp_double);
    printf("malloc per merge: %llu allocations, median %.1f ms\n", allocations, ta[2]);
    printf("one buffer:       1 allocation, median %.1f ms\n", tb[2]);
    printf("results identical\n");
    free(in); free(a); free(b); free(buf);
    return 0;
}
