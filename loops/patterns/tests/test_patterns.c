/* Tests for the pattern library: every pattern at every allowed size is
   checked against rules that describe its shape, not against stored text. */
#include "patterns.h"

#include <stdlib.h>
#include <string.h>

static int checks, failures;
#define CHECK(cond)                                                       \
    do {                                                                  \
        checks++;                                                         \
        if (!(cond)) {                                                    \
            failures++;                                                   \
            if (failures <= 20)                                           \
                fprintf(stderr, "FAILED line %d: %s\n", __LINE__, #cond); \
        }                                                                 \
    } while (0)

#define MAX_LINES 128
#define MAX_WIDTH 256

/* Runs a pattern and splits its output into lines. Returns the line count. */
static int capture(const pattern *p, int n, char lines[][MAX_WIDTH])
{
    FILE *f = tmpfile();
    if (f == NULL) {
        fprintf(stderr, "tmpfile failed\n");
        exit(1);
    }
    p->draw(f, n);
    rewind(f);
    int count = 0;
    char buf[MAX_WIDTH + 2];
    while (count < MAX_LINES && fgets(buf, sizeof buf, f) != NULL) {
        size_t len = strlen(buf);
        CHECK(len > 0 && buf[len - 1] == '\n');          /* every line is complete */
        if (len > 0 && buf[len - 1] == '\n')
            buf[--len] = '\0';
        CHECK(len < MAX_WIDTH);
        strcpy(lines[count++], buf);
    }
    fclose(f);
    return count;
}

static int leading(const char *s)
{
    int k = 0;
    while (s[k] == ' ')
        k++;
    return k;
}

static int count_char(const char *s, char c)
{
    int k = 0;
    for (; *s; s++)
        k += *s == c;
    return k;
}

/* Rows of a centered pyramid of height n: n - i spaces, then 2i - 1 stars. */
static int is_pyramid_row(const char *s, int n, int i)
{
    return leading(s) == n - i && (int)strlen(s) == n + i - 1 && count_char(s, '*') == 2 * i - 1;
}

static void test_shapes(void)
{
    static char lines[MAX_LINES][MAX_WIDTH];
    for (const pattern *p = patterns; p->name != NULL; p++)
        for (int n = 1; n <= p->max_n; n++) {
            int rows = capture(p, n, lines);
            for (int r = 0; r < rows; r++) {
                size_t len = strlen(lines[r]);
                CHECK(len > 0 && lines[r][len - 1] != ' ');   /* no trailing spaces */
            }
            const char *name = p->name;
            if (strcmp(name, "pyramid") == 0) {
                CHECK(rows == n);
                for (int i = 1; i <= rows; i++)
                    CHECK(is_pyramid_row(lines[i - 1], n, i));
            } else if (strcmp(name, "inverted-pyramid") == 0) {
                CHECK(rows == n);
                for (int i = 1; i <= rows; i++)
                    CHECK(is_pyramid_row(lines[i - 1], n, n + 1 - i));
            } else if (strcmp(name, "diamond") == 0 || strcmp(name, "hourglass") == 0) {
                CHECK(rows == 2 * n - 1);
                for (int r = 0; r < rows; r++) {
                    CHECK(strcmp(lines[r], lines[rows - 1 - r]) == 0);       /* symmetric */
                    int i = strcmp(name, "diamond") == 0 ? (r < n ? r + 1 : rows - r) : (r < n ? n - r : r - n + 2);
                    CHECK(is_pyramid_row(lines[r], n, i));
                }
            } else if (strcmp(name, "hollow-diamond") == 0 || strcmp(name, "hollow-pyramid") == 0) {
                int diamond = name[7] == 'd';
                CHECK(rows == (diamond ? 2 * n - 1 : n));
                for (int r = 0; r < rows; r++) {
                    int i = r < n ? r + 1 : rows - r;
                    int stars = count_char(lines[r], '*');
                    CHECK(leading(lines[r]) == n - i && (int)strlen(lines[r]) == n + i - 1);
                    if (!diamond && i == n)
                        CHECK(stars == 2 * n - 1);                 /* the base is solid */
                    else
                        CHECK(stars == (i == 1 ? 1 : 2));          /* otherwise only the edges */
                }
            } else if (strcmp(name, "square") == 0 || strcmp(name, "hollow-square") == 0) {
                CHECK(rows == n);
                for (int r = 0; r < rows; r++) {
                    CHECK((int)strlen(lines[r]) == n);
                    int edge = r == 0 || r == n - 1 || name[0] == 's';
                    CHECK(count_char(lines[r], '*') == (edge ? n : 2));
                }
            } else if (strcmp(name, "right-triangle") == 0 || strcmp(name, "mirrored-right-triangle") == 0) {
                CHECK(rows == n);
                for (int i = 1; i <= rows; i++) {
                    CHECK(count_char(lines[i - 1], '*') == i);
                    CHECK(leading(lines[i - 1]) == (name[0] == 'm' ? n - i : 0));
                }
            } else if (strcmp(name, "inverted-right-triangle") == 0) {
                CHECK(rows == n);
                for (int i = 1; i <= rows; i++)
                    CHECK((int)strlen(lines[i - 1]) == n + 1 - i && count_char(lines[i - 1], '*') == n + 1 - i);
            } else if (strcmp(name, "floyd") == 0) {
                CHECK(rows == n);
                long expect = 1;
                for (int r = 0; r < rows; r++) {
                    char *s = lines[r], *end;
                    int k = 0;
                    for (long v = strtol(s, &end, 10); end != s; v = strtol(s, &end, 10)) {
                        CHECK(v == expect++);
                        s = end;
                        k++;
                    }
                    CHECK(k == r + 1);
                }
            } else if (strcmp(name, "pascal") == 0) {
                CHECK(rows == n);
                for (int r = 0; r < rows; r++) {
                    char *s = lines[r], *end;
                    int k = 0;
                    for (unsigned long long v = strtoull(s, &end, 10); end != s; v = strtoull(s, &end, 10)) {
                        CHECK(v == pascal_entry(r, k));
                        s = end;
                        k++;
                    }
                    CHECK(k == r + 1);
                }
            } else if (strcmp(name, "alphabet-triangle") == 0) {
                CHECK(rows == n);
                for (int i = 1; i <= rows; i++)
                    CHECK(strncmp(lines[i - 1], "ABCDEFGHIJKLMNOPQRSTUVWXYZ", (size_t)i) == 0 && (int)strlen(lines[i - 1]) == i);
            } else {                                               /* the two number triangles */
                CHECK(rows == n);
                int repeated = name[0] == 'r';
                for (int i = 1; i <= rows; i++) {
                    char want[MAX_WIDTH];
                    int len = 0;
                    for (int j = 1; j <= i; j++)
                        len += snprintf(want + len, sizeof want - (size_t)len, j == 1 ? "%d" : " %d", repeated ? i : j);
                    CHECK(strcmp(lines[i - 1], want) == 0);
                }
            }
        }
}

static void test_pascal_entries(void)
{
    /* Rebuild rows 0..67 with the addition rule and compare. */
    static unsigned long long row[70], prev[70];
    for (int r = 0; r <= 67; r++) {
        for (int k = 0; k <= r; k++)
            row[k] = k == 0 || k == r ? 1 : prev[k - 1] + prev[k];
        for (int k = 0; k <= r; k++)
            CHECK(pascal_entry(r, k) == row[k]);
        memcpy(prev, row, sizeof row);
    }
    CHECK(pascal_entry(67, 33) == 14226520737620288370ULL);   /* the largest that fits */
    CHECK(pascal_entry(68, 34) == 0);                          /* does not fit: reported, not wrapped */
    CHECK(pascal_entry(68, 0) == 1 && pascal_entry(68, 1) == 68);
    CHECK(pascal_entry(5, -1) == 0 && pascal_entry(5, 6) == 0);
}

static void test_table(void)
{
    int count = 0;
    for (const pattern *p = patterns; p->name != NULL; p++, count++) {
        CHECK(pattern_find(p->name) == p);
        CHECK(p->max_n >= 1);
    }
    CHECK(count == 16);
    CHECK(pattern_find("Pyramid") == NULL && pattern_find("") == NULL);
}

int main(void)
{
    test_table();
    test_shapes();
    test_pascal_entries();
    if (failures) {
        printf("%d of %d checks failed\n", failures, checks);
        return 1;
    }
    printf("all %d checks passed\n", checks);
    return 0;
}
