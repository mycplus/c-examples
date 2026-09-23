#include <stdio.h>
#include <string.h>

#include "array_ops.h"

static int failures = 0;

static void check(int ok, const char *what)
{
    printf("  %-56s %s\n", what, ok ? "ok" : "FAIL");
    if (!ok) failures++;
}

int main(void)
{
    puts("search");
    {
        const int a[] = { 10, 20, 30 };
        size_t at = 999;
        check(array_find(a, 3, 10, &at) == 1 && at == 0, "finds the first element");
        check(array_find(a, 3, 30, &at) == 1 && at == 2, "finds the last element");
        check(array_find(a, 3, 99, &at) == 0,            "reports a missing value");
        check(array_find(a, 0, 10, &at) == 0,            "empty range finds nothing");
        check(array_find(a, 3, 20, NULL) == 1,           "accepts a NULL index pointer");
    }

    puts("\ninsertion");
    {
        int a[5] = { 1, 2, 3 };
        size_t n = 3;
        check(array_insert(a, &n, 5, 0, 0) == 0 && n == 4 && a[0] == 0 && a[3] == 3,
              "inserts at the front and shifts the rest right");
        check(array_insert(a, &n, 5, n, 9) == 0 && n == 5 && a[4] == 9,
              "pos == n appends");
        check(array_insert(a, &n, 5, 0, 7) == -1 && n == 5,
              "refuses to insert when at capacity");
    }
    {
        int a[5] = { 1, 2, 3 };
        size_t n = 3;
        check(array_insert(a, &n, 5, 4, 9) == -1 && n == 3,
              "refuses a position beyond the end");
    }
    {
        int a[3] = { 0 };
        size_t n = 0;
        check(array_insert(a, &n, 3, 0, 5) == 0 && n == 1 && a[0] == 5,
              "inserts into an empty array");
    }

    puts("\ndeletion");
    {
        int a[5] = { 1, 2, 3, 4 };
        size_t n = 4;
        check(array_remove(a, &n, 0) == 0 && n == 3 && a[0] == 2 && a[2] == 4,
              "removes the first element and shifts left");
        check(array_remove(a, &n, n - 1) == 0 && n == 2 && a[1] == 3,
              "removes the last element");
        check(array_remove(a, &n, 5) == -1 && n == 2,
              "refuses a position beyond the end");
    }
    {
        int a[3] = { 0 };
        size_t n = 0;
        check(array_remove(a, &n, 0) == -1 && n == 0,
              "refuses to remove from an empty array");
    }

    puts("\nround trip");
    {
        int a[8] = { 1, 2, 3, 4 };
        size_t n = 4;
        const int before[4] = { 1, 2, 3, 4 };
        array_insert(a, &n, 8, 2, 99);
        array_remove(a, &n, 2);
        check(n == 4 && memcmp(a, before, sizeof before) == 0,
              "insert then remove at the same index restores the array");
    }

    printf("\n%s\n", failures ? "FAILURES" : "all checks passed");
    return failures != 0;
}
