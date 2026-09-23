/* test_flag.c - bubble_sort_flag() must agree with bubble_sort() */
#include <stdio.h>
#include <string.h>

#define main bubble_sort_demo_main
#include "../src/bubble_sort.c"
#undef main
#include "../src/bubble_sort_flag.c"

int main(void)
{
    unsigned int rng = 777u;
    int a[50], b[50], failures = 0;

    for (int t = 0; t < 10000; t++) {
        size_t n = t % 51;                              /* 0..50 elements */
        for (size_t i = 0; i < n; i++) {
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            a[i] = b[i] = (int)(rng % 7) - 3;           /* many duplicates */
        }
        bubble_sort(a, n);
        bubble_sort_flag(b, n);
        if (n && memcmp(a, b, n * sizeof a[0]) != 0)
            failures++;
    }
    bubble_sort_flag(NULL, 0);
    printf("flag variant: %d failures\n", failures);
    return failures != 0;
}
