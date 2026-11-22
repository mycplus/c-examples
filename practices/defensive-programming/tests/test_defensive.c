/* Unit tests for defensive.c. Prints one line per failed check and returns
 * the number of failures. Usage: test_defensive DATA_DIR TEMP_DIR */
#include "../src/defensive.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks = 0;
static int failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        checks++;                                                         \
        if (!(cond)) {                                                    \
            failures++;                                                   \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
        }                                                                 \
    } while (0)

static void test_parse_long(void)
{
    static const struct {
        const char *text;
        dp_status expected;
        long value;
    } cases[] = {
        {"42", DP_OK, 42},           {"0", DP_OK, 0},
        {"100", DP_OK, 100},         {"  7", DP_OK, 7},
        {"+9", DP_OK, 9},            {"", DP_ERR_EMPTY, 0},
        {"   ", DP_ERR_EMPTY, 0},    {"abc", DP_ERR_NOT_A_NUMBER, 0},
        {"-", DP_ERR_NOT_A_NUMBER, 0}, {"12abc", DP_ERR_TRAILING, 0},
        {"7 ", DP_ERR_TRAILING, 0},  {"101", DP_ERR_RANGE, 0},
        {"-1", DP_ERR_RANGE, 0},     {"99999999999999999999", DP_ERR_RANGE, 0},
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        long value = -12345;
        const dp_status st = dp_parse_long(cases[i].text, 0, 100, &value);
        CHECK(st == cases[i].expected);
        /* The output is written on success only. */
        CHECK(value == (st == DP_OK ? cases[i].value : -12345));
    }
    long v;
    CHECK(dp_parse_long(NULL, 0, 1, &v) == DP_ERR_NULL);
    CHECK(dp_parse_long("1", 0, 1, NULL) == DP_ERR_NULL);
    CHECK(dp_parse_long("-9223372036854775808", LONG_MIN, LONG_MAX, &v) == (sizeof(long) == 8 ? DP_OK : DP_ERR_RANGE));
}

static void test_copy_string(void)
{
    char buf[6];
    CHECK(dp_copy_string(buf, sizeof buf, "hello") == DP_OK && strcmp(buf, "hello") == 0);
    CHECK(dp_copy_string(buf, sizeof buf, "hello!") == DP_ERR_TOO_LONG && strcmp(buf, "hello") == 0);
    CHECK(dp_copy_string(buf, sizeof buf, "") == DP_OK && buf[0] == '\0');
    CHECK(dp_copy_string(buf, 1, "x") == DP_ERR_TOO_LONG && buf[0] == '\0');
    CHECK(dp_copy_string(buf, 0, "x") == DP_ERR_TOO_LONG);
    CHECK(dp_copy_string(NULL, 4, "x") == DP_ERR_NULL);
}

static void test_sizes(void)
{
    size_t n = 0;
    CHECK(dp_mul_size(1000, 1000, &n) == DP_OK && n == 1000000);
    CHECK(dp_mul_size(SIZE_MAX, 1, &n) == DP_OK && n == SIZE_MAX);
    CHECK(dp_mul_size(SIZE_MAX / 2 + 1, 2, &n) == DP_ERR_OVERFLOW);
    CHECK(dp_mul_size(0, SIZE_MAX, &n) == DP_OK && n == 0);

    void *p = NULL;
    CHECK(dp_alloc_array(SIZE_MAX / 4 + 1, 8, &p) == DP_ERR_OVERFLOW && p == NULL);
    CHECK(dp_alloc_array(10, sizeof(int), &p) == DP_OK && p != NULL && ((int *)p)[9] == 0);
    free(p);
}

static void test_read_line(const char *data_dir)
{
    char path[512];
    snprintf(path, sizeof path, "%s/lines.txt", data_dir);
    FILE *f = fopen(path, "rb");
    CHECK(f != NULL);
    if (f == NULL) {
        return;
    }
    char buf[8];
    CHECK(dp_read_line(f, buf, sizeof buf) == DP_OK && strcmp(buf, "short") == 0);
    CHECK(dp_read_line(f, buf, sizeof buf) == DP_ERR_TOO_LONG && buf[0] == '\0');
    CHECK(dp_read_line(f, buf, sizeof buf) == DP_OK && strcmp(buf, "crlf") == 0);
    CHECK(dp_read_line(f, buf, sizeof buf) == DP_OK && strcmp(buf, "") == 0);
    CHECK(dp_read_line(f, buf, sizeof buf) == DP_OK && strcmp(buf, "6chars") == 0);
    /* Boundaries: 7 characters fill buf exactly; 8 do not fit. */
    CHECK(dp_read_line(f, buf, sizeof buf) == DP_OK && strcmp(buf, "abcdefg") == 0);
    CHECK(dp_read_line(f, buf, sizeof buf) == DP_OK && strcmp(buf, "abcdef") == 0);
    CHECK(dp_read_line(f, buf, sizeof buf) == DP_ERR_TOO_LONG);
    CHECK(dp_read_line(f, buf, sizeof buf) == DP_OK && strcmp(buf, "last") == 0);
    CHECK(dp_read_line(f, buf, sizeof buf) == DP_ERR_EOF);
    CHECK(dp_read_line(f, buf, 1) == DP_ERR_RANGE);
    fclose(f);
}

static void test_copy_file(const char *data_dir, const char *tmp_dir)
{
    char from[512], to[512], missing[512];
    snprintf(from, sizeof from, "%s/lines.txt", data_dir);
    snprintf(to, sizeof to, "%s/copy-of-lines.txt", tmp_dir);
    snprintf(missing, sizeof missing, "%s/no-such-file.txt", data_dir);

    CHECK(dp_copy_file(from, to) == DP_OK);
    FILE *a = fopen(from, "rb");
    FILE *b = fopen(to, "rb");
    CHECK(a != NULL && b != NULL);
    if (a != NULL && b != NULL) {
        int ca, cb;
        do {
            ca = getc(a);
            cb = getc(b);
        } while (ca == cb && ca != EOF);
        CHECK(ca == cb);
    }
    if (a != NULL) fclose(a);
    if (b != NULL) fclose(b);

    /* A missing source fails with errno set and leaves an existing target alone. */
    errno = 0;
    CHECK(dp_copy_file(missing, to) == DP_ERR_IO && errno == ENOENT);
    FILE *still = fopen(to, "rb");
    CHECK(still != NULL);
    if (still != NULL) fclose(still);
    remove(to);
}

static void test_strerror(void)
{
    for (int s = DP_OK; s <= DP_ERR_IO; s++) {
        CHECK(strcmp(dp_strerror((dp_status)s), "unknown error") != 0);
    }
    CHECK(strcmp(dp_strerror((dp_status)99), "unknown error") == 0);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s DATA_DIR TEMP_DIR\n", argv[0]);
        return 2;
    }
    test_parse_long();
    test_copy_string();
    test_sizes();
    test_read_line(argv[1]);
    test_copy_file(argv[1], argv[2]);
    test_strerror();
    printf("%d of %d checks passed\n", checks - failures, checks);
    return failures == 0 ? 0 : 1;
}
