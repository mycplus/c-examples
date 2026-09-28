/* growth.c - how the growth policy of an array-backed stack changes the
 * number of reallocations and element copies for one million pushes.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic growth.c -o growth
 */
#include <stdio.h>
#include <stdlib.h>

enum { PUSHES = 1000000, START = 8 };

typedef size_t (*GrowFn)(size_t cap);

static size_t add_16(size_t cap)     { return cap + 16; }
static size_t add_1024(size_t cap)   { return cap + 1024; }
static size_t times_1_5(size_t cap)  { return cap + cap / 2; }
static size_t times_2(size_t cap)    { return cap * 2; }

/* Pushes PUSHES ints, growing with grow(). Counts resizes, and the elements
 * a copying reallocation must move (the old size at each resize). */
static int run(const char *name, GrowFn grow)
{
    size_t cap = START, size = 0, resizes = 0;
    unsigned long long copied = 0;
    int *items = malloc(cap * sizeof *items);
    if (items == NULL)
        return -1;
    for (int i = 0; i < PUSHES; ++i) {
        if (size == cap) {
            size_t new_cap = grow(cap);
            int *p = realloc(items, new_cap * sizeof *p);
            if (p == NULL) {
                free(items);
                return -1;
            }
            items = p;
            cap = new_cap;
            copied += size;
            resizes++;
        }
        items[size++] = i;
    }
    printf("%-10s %9zu %15llu %10.2f %10zu\n", name, resizes, copied,
           (double)copied / PUSHES, cap);
    free(items);
    return 0;
}

int main(void)
{
    printf("%-10s %9s %15s %10s %10s\n",
           "policy", "resizes", "elements moved", "per push", "final cap");
    if (run("+16", add_16) || run("+1024", add_1024) ||
        run("x1.5", times_1_5) || run("x2", times_2)) {
        fputs("out of memory\n", stderr);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
