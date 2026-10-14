/* matrix_sum.c - sums a 256 x 256 matrix of ints row by row or column by
 * column. A real program to trace with Valgrind:
 *   valgrind --tool=lackey --trace-mem=yes --log-file=col.trace ./matrix_sum col */
#include <stdio.h>
#include <string.h>

#define N 256
static int m[N][N];

int main(int argc, char **argv)
{
    if (argc != 2 || (strcmp(argv[1], "row") != 0 && strcmp(argv[1], "col") != 0)) {
        fprintf(stderr, "usage: matrix_sum row|col\n");
        return 2;
    }
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            m[i][j] = i + j;

    long long sum = 0;
    if (strcmp(argv[1], "row") == 0) {
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++)
                sum += m[i][j];
    } else {
        for (int j = 0; j < N; j++)
            for (int i = 0; i < N; i++)
                sum += m[i][j];
    }
    printf("sum = %lld\n", sum);
    return 0;
}
