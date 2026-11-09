/* Tests for src/callbacks.c and src/generic_swap.c, included with main() renamed. */
#define main callbacks_demo_main
#include "../src/callbacks.c"
#undef main
#define main generic_swap_demo_main
#include "../src/generic_swap.c"
#undef main

#include <stdbool.h>

static int failures = 0;
static void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, line, expr);
        failures++;
    }
}
#define CHECK(cond) check((cond), #cond, __LINE__)

static void test_sort_order(void)
{
    static const char *names[] = {"Ada", "Bo", "Cy", "Di", "Ed", "Flo", "Gus", "Hal"};
    struct employee staff[200];
    unsigned long long state = 12345;
    for (int i = 0; i < 200; i++) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        staff[i].name = names[(state >> 33) % 8];
        staff[i].salary = (int)((state >> 40) % 5) * 1000;   /* few distinct salaries: many ties */
    }
    qsort(staff, 200, sizeof staff[0], by_salary_desc);
    for (int i = 1; i < 200; i++) {
        CHECK(staff[i - 1].salary >= staff[i].salary);
        if (staff[i - 1].salary == staff[i].salary)
            CHECK(strcmp(staff[i - 1].name, staff[i].name) <= 0);
    }
}

static void test_operations(void)
{
    CHECK(sizeof operations / sizeof operations[0] == 4);
    CHECK(operations[0].apply(2, 3) == 5);
    CHECK(operations[1].apply(2, 3) == -1);
    CHECK(operations[2].apply(2, 3) == 6);
    CHECK(operations[3].apply(6, 3) == 2);
    CHECK(operations[3].apply(6, 0) == 0.0);      /* divide by zero is handled, not executed */
}

static void test_swap_bytes(void)
{
    int a = 1, b = 2;
    swap_bytes(&a, &b, sizeof a);
    CHECK(a == 2 && b == 1);

    struct point p = {1, 2}, q = {3, 4};
    swap_bytes(&p, &q, sizeof p);
    CHECK(p.x == 3 && p.y == 4 && q.x == 1 && q.y == 2);

    int same = 5;
    swap_bytes(&same, &same, sizeof same);        /* same object: unchanged */
    CHECK(same == 5);

    int keep = 8, other = 9;
    swap_bytes(&keep, &other, 0);                 /* zero bytes: nothing moves */
    CHECK(keep == 8 && other == 9);
}

int main(void)
{
    test_sort_order();
    test_operations();
    test_swap_bytes();
    if (failures == 0)
        printf("test_generic: all checks passed\n");
    return failures != 0;
}
