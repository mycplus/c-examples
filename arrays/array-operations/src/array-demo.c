/* array-demo.c - the array operations, each run once.
 *
 *   cc -std=c11 -Wall -Wextra -pedantic -Iinclude \
 *      src/array_ops.c src/array-demo.c -o array-demo
 */
#include <stdio.h>

#include "array_ops.h"

#define CAPACITY 10
#define COLS      3

/* An array argument decays to a pointer, so the callee cannot recover the
   element count. It has to be passed. */
static size_t count_from_pointer(const int *a)
{
    (void)a;
    return sizeof a;            /* the size of a pointer, not of the array */
}

/* For a 2D array the column count is part of the parameter type: the
   compiler needs it to work out where row r starts. The parameter is not
   const-qualified because before C23, ISO C does not allow passing
   int (*)[N] to a const int (*)[N] parameter. */
static void matrix_print(int m[][COLS], size_t rows)
{
    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < COLS; ++c)
            printf("%4d", m[r][c]);
        putchar('\n');
    }
}

int main(void)
{
    int  data[CAPACITY] = { 10, 20, 30, 40, 50 };
    size_t n = 5;                       /* elements in use, not capacity */

    puts("-- traverse");
    array_print(data, n);

    puts("\n-- access and update");
    printf("data[2] is %d\n", data[2]);
    data[2] = 35;
    printf("after data[2] = 35: ");
    array_print(data, n);

    puts("\n-- linear search");
    size_t at;
    printf("40 found: %s", array_find(data, n, 40, &at) ? "yes" : "no");
    printf(", index %zu\n", at);
    printf("99 found: %s\n", array_find(data, n, 99, &at) ? "yes" : "no");

    puts("\n-- insertion");
    array_insert(data, &n, CAPACITY, 2, 25);
    printf("insert 25 at index 2: ");
    array_print(data, n);
    array_insert(data, &n, CAPACITY, n, 60);
    printf("append 60:            ");
    array_print(data, n);

    puts("\n-- deletion");
    array_remove(data, &n, 0);
    printf("remove index 0:       ");
    array_print(data, n);

    puts("\n-- array size");
    printf("sizeof data = %zu bytes, %zu elements\n",
           sizeof data, sizeof data / sizeof data[0]);
    printf("same array seen through a parameter: %zu bytes\n",
           count_from_pointer(data));
    printf("elements in use: %zu of %d\n", n, CAPACITY);

    puts("\n-- multi-dimensional");
    int grid[2][COLS] = { { 1, 2, 3 }, { 4, 5, 6 } };
    matrix_print(grid, 2);
    printf("grid[1][2] is %d\n", grid[1][2]);
    grid[1][2] = 60;
    printf("after grid[1][2] = 60:\n");
    matrix_print(grid, 2);
    printf("sizeof grid = %zu, sizeof grid[0] = %zu, rows = %zu\n",
           sizeof grid, sizeof grid[0], sizeof grid / sizeof grid[0]);

    return 0;
}
