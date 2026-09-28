/* Tests for src/linked_stack.c, included with main() renamed. */
#define main linked_stack_demo_main
#include "../src/linked_stack.c"
#undef main

#include <limits.h>

static int failures = 0;
static void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, line, expr);
        failures++;
    }
}
#define CHECK(cond) check((cond), #cond, __LINE__)

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
    LinkedStack s;
    lstack_init(&s);
    int out = 12345;
    CHECK(lstack_is_empty(&s));
    CHECK(!lstack_pop(&s, &out));
    CHECK(!lstack_peek(&s, &out));
    CHECK(out == 12345);
    lstack_free(&s);
}

static void test_differential(void)
{
    enum { OPS = 200000, MODEL_MAX = 200000 };
    static int model[MODEL_MAX];
    size_t n = 0;
    LinkedStack s;
    lstack_init(&s);
    for (int i = 0; i < OPS; ++i) {
        unsigned long long r = next_rand();
        if (r % 3 != 0) {
            int v = (r % 17 == 0) ? INT_MIN : (r % 19 == 0) ? INT_MAX
                                             : (int)(r >> 33);
            CHECK(lstack_push(&s, v));
            model[n++] = v;
        } else {
            int out = 0;
            bool got = lstack_pop(&s, &out);
            CHECK(got == (n > 0));
            if (n > 0)
                CHECK(out == model[--n]);
        }
        CHECK(s.size == n);
    }
    lstack_free(&s);                        /* frees the remaining nodes */
    CHECK(lstack_is_empty(&s) && s.size == 0);
}

/* One million nodes, then free: must not recurse and must not leak
 * (the sanitizer job runs this under LeakSanitizer). */
static void test_large_free(void)
{
    LinkedStack s;
    lstack_init(&s);
    for (int i = 0; i < 1000000; ++i)
        CHECK(lstack_push(&s, i));
    CHECK(s.size == 1000000);
    int top = 0;
    CHECK(lstack_peek(&s, &top) && top == 999999);
    lstack_free(&s);
    CHECK(s.top == NULL);
}

int main(void)
{
    test_empty();
    test_differential();
    test_large_free();
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    puts("linked_stack: all tests passed");
    return EXIT_SUCCESS;
}
