/* Language-level facts the article relies on, checked on the machine running the tests. */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

static int failures = 0;
static void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, line, expr);
        failures++;
    }
}
#define CHECK(cond) check((cond), #cond, __LINE__)

struct reading {
    char sensor;
    double value;
};

static size_t size_of_param(const int *a) { return sizeof a; }

int main(void)
{
    int a[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    double d[4] = {0};
    struct reading r[3];
    char c[3];

    /* p + 1 moves by sizeof(*p) bytes, for every element type */
    CHECK((char *)(a + 1) - (char *)a == (ptrdiff_t)sizeof a[0]);
    CHECK((char *)(d + 1) - (char *)d == (ptrdiff_t)sizeof d[0]);
    CHECK((char *)(r + 1) - (char *)r == (ptrdiff_t)sizeof r[0]);
    CHECK((char *)(c + 1) - (char *)c == 1);

    /* pointer difference counts elements, not bytes */
    CHECK(&a[7] - &a[2] == 5);

    /* a[i] and *(a + i) are the same lvalue */
    for (int i = 0; i < 8; i++) {
        CHECK(&a[i] == a + i);
        CHECK(a[i] == *(a + i));
    }

    /* an array passed to a function arrives as a pointer: its length is gone */
    CHECK(sizeof a == 8 * sizeof(int));
    CHECK(size_of_param(a) == sizeof(int *));

    /* every object pointer in this program has the same size */
    CHECK(sizeof(char *) == sizeof(double *));
    CHECK(sizeof(char *) == sizeof(struct reading *));
    CHECK(sizeof(char *) == sizeof(void *));

    /* one-past-the-end is a valid pointer to compare with, and the loop ends on it */
    int total = 0;
    for (const int *p = a, *end = a + 8; p != end; p++)
        total += *p;
    CHECK(total == 28);

    if (failures == 0)
        printf("test_pointer_arith: all checks passed\n");
    return failures != 0;
}
