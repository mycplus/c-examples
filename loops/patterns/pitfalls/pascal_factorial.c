/* Pitfall: computing Pascal's triangle with factorials.
   C(r, k) = r! / (k! (r - k)!) is the textbook formula, but r! outgrows a
   64-bit integer at r = 21 (21! > 2^64), long before the entries do: the
   biggest entry of row 21 is only 352,716. Unsigned arithmetic wraps
   around instead of failing, so the program prints wrong numbers without
   any warning. The multiplicative formula used by patterns.c stays exact
   up to row 67. */
#include "patterns.h"

#include <stdio.h>

static unsigned long long factorial(int n)
{
    unsigned long long f = 1;
    for (int i = 2; i <= n; i++)
        f *= (unsigned long long)i;          /* wraps silently past 20! */
    return f;
}

static unsigned long long by_factorial(int r, int k)
{
    return factorial(r) / (factorial(k) * factorial(r - k));
}

int main(void)
{
    printf("%-4s %-4s %22s %22s\n", "row", "k", "factorials", "multiplicative");
    int rows[] = {20, 21, 22, 25};
    for (int i = 0; i < 4; i++) {
        int r = rows[i], k = r / 2;
        unsigned long long a = by_factorial(r, k), b = pascal_entry(r, k);
        printf("%-4d %-4d %22llu %22llu%s\n", r, k, a, b, a == b ? "" : "  <- wrong");
    }
    return 0;
}
