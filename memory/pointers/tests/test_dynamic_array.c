/* Tests for src/dynamic_array.c, included with main() renamed. */
#define main dynamic_array_demo_main
#include "../src/dynamic_array.c"
#undef main

static int failures = 0;
static void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, line, expr);
        failures++;
    }
}
#define CHECK(cond) check((cond), #cond, __LINE__)

static void test_growth_keeps_contents(void)
{
    IntList l = {0};
    for (int i = 0; i < 1000; i++)
        CHECK(list_push(&l, i * 3));
    CHECK(l.len == 1000);
    CHECK(l.cap == 1024);                         /* 4, 8, 16, ... 1024 */
    for (int i = 0; i < 1000; i++)
        CHECK(l.data[i] == i * 3);
    list_free(&l);
    CHECK(l.data == NULL && l.len == 0 && l.cap == 0);
    list_free(&l);                                /* freeing twice through the wrapper is safe */
}

static void test_capacity_overflow_is_refused(void)
{
    IntList l = {0};
    l.cap = SIZE_MAX / 2 + 1;                     /* doubling would overflow size_t */
    l.len = l.cap;
    CHECK(!list_push(&l, 1));
    CHECK(l.data == NULL && l.len == l.cap);      /* nothing was touched */
}

int main(void)
{
    test_growth_keeps_contents();
    test_capacity_overflow_is_refused();
    if (failures == 0)
        printf("test_dynamic_array: all checks passed\n");
    return failures != 0;
}
