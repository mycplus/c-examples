/* merge_demo.c - merge sort on 8 elements, one level of merges per line,
 * with the comparisons each merge makes */
#include <stdio.h>
#include <string.h>

#define N 8

int main(void)
{
    int a[N] = { 29, 10, 14, 37, 13, 5, 41, 22 }, buf[N];
    int total = 0;

    for (size_t width = 1; width < N; width *= 2) {
        int cmps[N] = { 0 };
        size_t runs = 0;
        for (size_t lo = 0; lo < N; lo += 2 * width, runs++) {   /* one merge */
            size_t mid = lo + width, hi = lo + 2 * width;
            size_t i = lo, j = mid, k = lo;
            while (i < mid && j < hi) {
                cmps[runs]++;
                buf[k++] = (a[j] < a[i]) ? a[j++] : a[i++];
            }
            while (i < mid) buf[k++] = a[i++];
            while (j < hi)  buf[k++] = a[j++];
        }
        memcpy(a, buf, sizeof a);
        printf("runs of %zu:", 2 * width);
        for (size_t r = 0; r < runs; r++) {
            printf("  [");
            for (size_t x = r * 2 * width; x < (r + 1) * 2 * width; x++)
                printf(x == r * 2 * width ? "%d" : " %d", a[x]);
            printf("] %d cmp", cmps[r]);
            total += cmps[r];
        }
        printf("\n");
    }
    printf("total comparisons: %d\n", total);
    return 0;
}
