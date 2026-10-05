/* stack_table.c - the full (n + 1) x (W + 1) table as a variable-length
   array on the stack. Correct for small inputs; at 100 items and capacity
   100,000 the table is 40,400,404 bytes and overflows a typical 8 MiB stack.
   Not built by CMake (MSVC does not support VLAs); the workflow checks that
   AddressSanitizer reports the overflow. */
#include <stdio.h>

static int knapsack_vla(int capacity, int n, const int weight[], const int value[])
{
    int dp[n + 1][capacity + 1];

    for (int i = 0; i <= n; i++) {
        for (int c = 0; c <= capacity; c++) {
            if (i == 0 || c == 0)
                dp[i][c] = 0;
            else if (weight[i - 1] <= c && value[i - 1] + dp[i - 1][c - weight[i - 1]] > dp[i - 1][c])
                dp[i][c] = value[i - 1] + dp[i - 1][c - weight[i - 1]];
            else
                dp[i][c] = dp[i - 1][c];
        }
    }
    return dp[n][capacity];
}

int main(void)
{
    enum { N = 100 };
    int weight[N], value[N];
    for (int i = 0; i < N; i++) {
        weight[i] = 1000 + i;
        value[i] = i + 1;
    }
    printf("small: %d\n", knapsack_vla(50, 4, (const int[]){10, 20, 30, 5},
                                        (const int[]){60, 100, 120, 50}));
    fflush(stdout);
    printf("large: %d\n", knapsack_vla(100000, N, weight, value));
    return 0;
}
