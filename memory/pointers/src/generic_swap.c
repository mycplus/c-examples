#include <stdio.h>

/* Swaps any two objects of the same size, one byte at a time. */
static void swap_bytes(void *a, void *b, size_t size)
{
    unsigned char *x = a;
    unsigned char *y = b;
    while (size--) {
        unsigned char tmp = *x;
        *x++ = *y;
        *y++ = tmp;
    }
}

struct point {
    int x, y;
};

int main(void)
{
    int i = 1, j = 2;
    double d = 1.5, e = 9.25;
    struct point p = {1, 2}, q = {30, 40};

    swap_bytes(&i, &j, sizeof i);
    swap_bytes(&d, &e, sizeof d);
    swap_bytes(&p, &q, sizeof p);

    printf("ints:    %d %d\n", i, j);
    printf("doubles: %.2f %.2f\n", d, e);
    printf("points:  (%d,%d) (%d,%d)\n", p.x, p.y, q.x, q.y);
    return 0;
}
