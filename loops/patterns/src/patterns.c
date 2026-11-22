#include "patterns.h"

#include <stdint.h>
#include <string.h>

static void repeat(FILE *out, char c, int count)
{
    for (int i = 0; i < count; i++)
        fputc(c, out);
}

/* ---- Squares and triangles ---- */

static void square(FILE *out, int n)
{
    for (int i = 0; i < n; i++) {
        repeat(out, '*', n);
        fputc('\n', out);
    }
}

static void hollow_square(FILE *out, int n)
{
    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= n; j++)
            fputc(i == 1 || i == n || j == 1 || j == n ? '*' : ' ', out);
        fputc('\n', out);
    }
}

static void right_triangle(FILE *out, int n)            /* half pyramid */
{
    for (int i = 1; i <= n; i++) {
        repeat(out, '*', i);
        fputc('\n', out);
    }
}

static void inverted_right_triangle(FILE *out, int n)
{
    for (int i = n; i >= 1; i--) {
        repeat(out, '*', i);
        fputc('\n', out);
    }
}

static void mirrored_right_triangle(FILE *out, int n)   /* right-aligned */
{
    for (int i = 1; i <= n; i++) {
        repeat(out, ' ', n - i);
        repeat(out, '*', i);
        fputc('\n', out);
    }
}

/* ---- Pyramids and diamonds ---- */

/* One row of a centered pyramid of height n: n - i spaces, 2i - 1 stars. */
static void pyramid_row(FILE *out, int n, int i)
{
    repeat(out, ' ', n - i);
    repeat(out, '*', 2 * i - 1);
    fputc('\n', out);
}

static void pyramid(FILE *out, int n)
{
    for (int i = 1; i <= n; i++)
        pyramid_row(out, n, i);
}

static void inverted_pyramid(FILE *out, int n)
{
    for (int i = n; i >= 1; i--)
        pyramid_row(out, n, i);
}

static void hollow_pyramid(FILE *out, int n)
{
    for (int i = 1; i <= n; i++) {
        repeat(out, ' ', n - i);
        for (int k = 1; k <= 2 * i - 1; k++)
            fputc(k == 1 || k == 2 * i - 1 || i == n ? '*' : ' ', out);
        fputc('\n', out);
    }
}

/* A diamond is a pyramid of n rows on top of an inverted pyramid of n - 1:
   the widest row is printed once, not twice. */
static void diamond(FILE *out, int n)
{
    for (int i = 1; i <= n; i++)
        pyramid_row(out, n, i);
    for (int i = n - 1; i >= 1; i--)
        pyramid_row(out, n, i);
}

static void hollow_diamond_row(FILE *out, int n, int i)
{
    repeat(out, ' ', n - i);
    for (int k = 1; k <= 2 * i - 1; k++)
        fputc(k == 1 || k == 2 * i - 1 ? '*' : ' ', out);
    fputc('\n', out);
}

static void hollow_diamond(FILE *out, int n)
{
    for (int i = 1; i <= n; i++)
        hollow_diamond_row(out, n, i);
    for (int i = n - 1; i >= 1; i--)
        hollow_diamond_row(out, n, i);
}

static void hourglass(FILE *out, int n)
{
    for (int i = n; i >= 1; i--)
        pyramid_row(out, n, i);
    for (int i = 2; i <= n; i++)
        pyramid_row(out, n, i);
}

/* ---- Number and letter patterns ---- */

static void number_triangle(FILE *out, int n)            /* 1, 12, 123 */
{
    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= i; j++)
            fprintf(out, j == 1 ? "%d" : " %d", j);
        fputc('\n', out);
    }
}

static void repeated_number_triangle(FILE *out, int n)   /* 1, 22, 333 */
{
    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= i; j++)
            fprintf(out, j == 1 ? "%d" : " %d", i);
        fputc('\n', out);
    }
}

static int digits(long long v)
{
    int d = 1;
    while (v >= 10) {
        v /= 10;
        d++;
    }
    return d;
}

/* Floyd's triangle: consecutive numbers, row i holds i of them. Every
   column is as wide as the largest number, so the columns line up. */
static void floyd(FILE *out, int n)
{
    int width = digits((long long)n * (n + 1) / 2), next = 1;
    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= i; j++)
            fprintf(out, j == 1 ? "%*d" : " %*d", width, next++);
        fputc('\n', out);
    }
}

static unsigned long long gcd(unsigned long long a, unsigned long long b)
{
    while (b != 0) {
        unsigned long long t = a % b;
        a = b;
        b = t;
    }
    return a;
}

unsigned long long pascal_entry(int r, int k)
{
    if (k < 0 || k > r)
        return 0;
    if (k > r - k)
        k = r - k;                       /* C(r, k) = C(r, r - k) */
    unsigned long long c = 1;
    for (int j = 1; j <= k; j++) {
        /* c = c * m / j, where the division is exact. Dividing first keeps
           the intermediate value no larger than the result: with g =
           gcd(m, j), j / g has no factor in common with m / g, so it must
           divide c. */
        unsigned long long m = (unsigned long long)(r - k + j), g = gcd(m, (unsigned long long)j);
        unsigned long long q = c / ((unsigned long long)j / g), f = m / g;
        if (q > UINT64_MAX / f)
            return 0;                    /* the entry does not fit in 64 bits */
        c = q * f;
    }
    return c;
}

/* Pascal's triangle, centered. Every entry is printed right-aligned in a
   cell of the same even width w, and row r is indented by (n - 1 - r)
   half-cells, so each entry sits between the two entries above it. */
static void pascal(FILE *out, int n)
{
    int w = digits((long long)pascal_entry(n - 1, (n - 1) / 2)) + 1;
    if (w % 2)
        w++;
    for (int r = 0; r < n; r++) {
        repeat(out, ' ', (n - 1 - r) * w / 2);
        for (int k = 0; k <= r; k++)
            fprintf(out, "%*llu", k == 0 ? w / 2 : w, pascal_entry(r, k));
        fputc('\n', out);
    }
}

static void alphabet_triangle(FILE *out, int n)          /* A, AB, ABC */
{
    static const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (int i = 1; i <= n; i++) {
        fwrite(letters, 1, (size_t)i, out);
        fputc('\n', out);
    }
}

const pattern patterns[] = {
    {"square", square, 50, "n x n block of stars"},
    {"hollow-square", hollow_square, 50, "only the border of the square"},
    {"right-triangle", right_triangle, 50, "half pyramid: 1, 2, 3 ... stars"},
    {"inverted-right-triangle", inverted_right_triangle, 50, "n, n-1 ... 1 stars"},
    {"mirrored-right-triangle", mirrored_right_triangle, 50, "half pyramid aligned to the right"},
    {"pyramid", pyramid, 50, "centered: 1, 3, 5 ... stars"},
    {"inverted-pyramid", inverted_pyramid, 50, "pyramid upside down"},
    {"hollow-pyramid", hollow_pyramid, 50, "outline of the pyramid"},
    {"diamond", diamond, 50, "pyramid + inverted pyramid, 2n - 1 rows"},
    {"hollow-diamond", hollow_diamond, 50, "outline of the diamond"},
    {"hourglass", hourglass, 50, "inverted pyramid + pyramid"},
    {"number-triangle", number_triangle, 9, "1 / 1 2 / 1 2 3"},
    {"repeated-number-triangle", repeated_number_triangle, 9, "1 / 2 2 / 3 3 3"},
    {"floyd", floyd, 30, "Floyd's triangle: consecutive numbers"},
    {"pascal", pascal, 20, "Pascal's triangle: binomial coefficients"},
    {"alphabet-triangle", alphabet_triangle, 26, "A / AB / ABC"},
    {NULL, NULL, 0, NULL},
};

const pattern *pattern_find(const char *name)
{
    for (const pattern *p = patterns; p->name != NULL; p++)
        if (strcmp(p->name, name) == 0)
            return p;
    return NULL;
}
