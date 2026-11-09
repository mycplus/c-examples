/* Tests for src/strings.c and src/double_pointer.c, included with main() renamed. */
#define main strings_demo_main
#include "../src/strings.c"
#undef main
#define main double_pointer_demo_main
#include "../src/double_pointer.c"
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

static void test_count_words(void)
{
    CHECK(count_words("") == 0);
    CHECK(count_words("   ") == 0);
    CHECK(count_words("a") == 1);
    CHECK(count_words(" a b ") == 2);
    CHECK(count_words("one\ttwo\nthree") == 3);
    CHECK(count_words("\xc3\xa9t\xc3\xa9 ok") == 2);   /* bytes above 127 must not trip isspace */
}

static void test_upper_copy(void)
{
    char *out = NULL;
    CHECK(upper_copy("mixed Case 123", &out));
    CHECK(out != NULL && strcmp(out, "MIXED CASE 123") == 0);
    free(out);

    out = NULL;
    CHECK(upper_copy("", &out));
    CHECK(out != NULL && out[0] == '\0');
    free(out);
}

int main(void)
{
    test_count_words();
    test_upper_copy();
    if (failures == 0)
        printf("test_text: all checks passed\n");
    return failures != 0;
}
