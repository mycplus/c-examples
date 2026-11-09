/* Tests for src/functions.c, included with main() renamed. */
#define main functions_demo_main
#include "../src/functions.c"
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

static void test_swap(void)
{
    int a = 1, b = 2;
    swap_int(&a, &b);
    CHECK(a == 2 && b == 1);
    swap_int(&a, &a);              /* aliasing: swapping with itself changes nothing */
    CHECK(a == 2);
}

static void test_stats(void)
{
    double one[] = {-3.5};
    double lo, hi, avg;
    stats(one, 1, &lo, &hi, &avg);
    CHECK(lo == -3.5 && hi == -3.5 && avg == -3.5);

    double v[] = {5.0, -2.0, 9.0, 0.0};
    stats(v, 4, &lo, &hi, &avg);
    CHECK(lo == -2.0 && hi == 9.0 && avg == 3.0);
}

static void test_parse_int(void)
{
    int out = 123;
    CHECK(parse_int("42", &out) && out == 42);
    CHECK(parse_int("-7", &out) && out == -7);
    CHECK(parse_int("+5", &out) && out == 5);
    CHECK(parse_int("2147483647", &out) && out == INT_MAX);
    CHECK(parse_int("-2147483648", &out) && out == INT_MIN);

    out = 999;                     /* every failure must leave *out untouched */
    CHECK(!parse_int("", &out) && out == 999);
    CHECK(!parse_int("12abc", &out) && out == 999);
    CHECK(!parse_int("abc", &out) && out == 999);
    CHECK(!parse_int("7 ", &out) && out == 999);
    CHECK(!parse_int("0x10", &out) && out == 999);
    CHECK(!parse_int("2147483648", &out) && out == 999);
    CHECK(!parse_int("-2147483649", &out) && out == 999);
    CHECK(!parse_int("99999999999999999999999", &out) && out == 999);
}

int main(void)
{
    test_swap();
    test_stats();
    test_parse_int();
    if (failures == 0)
        printf("test_functions: all checks passed\n");
    return failures != 0;
}
