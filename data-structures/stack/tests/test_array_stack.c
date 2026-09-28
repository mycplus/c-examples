/* Tests for src/array_stack.c. The source file is included with main()
 * renamed, so the tests exercise the same file the article shows. */
#define main array_stack_demo_main
#include "../src/array_stack.c"
#undef main

#include <limits.h>
#include <string.h>

static int failures = 0;
static void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, line, expr);
        failures++;
    }
}
#define CHECK(cond) check((cond), #cond, __LINE__)

/* Small deterministic generator so every platform sees the same sequence. */
static unsigned long long rng_state = 88172645463325252ULL;
static unsigned long long next_rand(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

static void test_empty(void)
{
    IntStack s;
    stack_init(&s);
    int out = 12345;
    CHECK(stack_is_empty(&s));
    CHECK(!stack_pop(&s, &out));
    CHECK(!stack_peek(&s, &out));
    CHECK(out == 12345);                    /* untouched on failure */
    stack_free(&s);
}

static void test_growth(void)
{
    IntStack s;
    stack_init(&s);
    CHECK(s.capacity == 0 && s.items == NULL);
    CHECK(stack_push(&s, 1));
    CHECK(s.capacity == 8);
    for (int i = 2; i <= 9; ++i)
        CHECK(stack_push(&s, i));
    CHECK(s.capacity == 16 && s.size == 9);
    stack_free(&s);
    CHECK(s.items == NULL && s.size == 0 && s.capacity == 0);
}

/* A capacity this large cannot double without overflowing the byte count.
 * The push must fail before calling realloc and leave the stack unchanged. */
static void test_overflow_guard(void)
{
    IntStack s;
    stack_init(&s);
    s.capacity = SIZE_MAX / 2 / sizeof *s.items + 1;
    s.size = s.capacity;
    CHECK(!stack_push(&s, 7));
    CHECK(s.items == NULL);
    CHECK(s.size == SIZE_MAX / 2 / sizeof *s.items + 1);
    stack_init(&s);                         /* nothing was allocated */
}

/* Random pushes and pops, checked against a plain array model. */
static void test_differential(void)
{
    enum { OPS = 200000, MODEL_MAX = 200000 };
    static int model[MODEL_MAX];
    size_t n = 0;
    IntStack s;
    stack_init(&s);
    for (int i = 0; i < OPS; ++i) {
        unsigned long long r = next_rand();
        if (r % 3 != 0) {                   /* push twice as often as pop */
            int v = (r % 17 == 0) ? INT_MIN : (r % 19 == 0) ? INT_MAX
                                             : (int)(r >> 33);
            CHECK(stack_push(&s, v));
            model[n++] = v;
        } else {
            int out = 0;
            bool got = stack_pop(&s, &out);
            CHECK(got == (n > 0));
            if (n > 0)
                CHECK(out == model[--n]);
        }
        CHECK(s.size == n);
    }
    int out = 0;
    while (n > 0) {
        CHECK(stack_pop(&s, &out) && out == model[--n]);
    }
    CHECK(stack_is_empty(&s));
    stack_free(&s);
}

static void test_balanced(void)
{
    CHECK(balanced(""));
    CHECK(balanced("()[]{}"));
    CHECK(balanced("{[()()]}"));
    CHECK(balanced("int main(void) { return a[(1)]; }"));
    CHECK(!balanced("("));
    CHECK(!balanced(")"));
    CHECK(!balanced("([)]"));
    CHECK(!balanced("(("));
    CHECK(!balanced("())"));
    CHECK(!balanced("}{"));

    enum { DEPTH = 100000 };
    static char deep[2 * DEPTH + 2];
    memset(deep, '(', DEPTH);
    memset(deep + DEPTH, ')', DEPTH);
    deep[2 * DEPTH] = '\0';
    CHECK(balanced(deep));
    deep[2 * DEPTH - 1] = ']';
    CHECK(!balanced(deep));
}

int main(void)
{
    test_empty();
    test_growth();
    test_overflow_guard();
    test_differential();
    test_balanced();
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    puts("array_stack: all tests passed");
    return EXIT_SUCCESS;
}
