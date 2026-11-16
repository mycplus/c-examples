/* off_by_one.c - a linear search whose loop runs one step too far,
 * i <= n instead of i < n.
 *
 *   off_by_one         the array sits in a struct, so the int "after" it is
 *                      the next field, and a missing key is found there
 *   off_by_one heap    the same search on a copy in its own heap block */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct record {
    int scores[5];
    int id;          /* the int right after the array */
};

static int search(const int *a, int n, int key)
{
    for (int i = 0; i <= n; i++)       /* should be i < n */
        if (a[i] == key)
            return i;
    return -1;
}

int main(int argc, char **argv)
{
    struct record r = {{12, 47, 33, 90, 61}, 99};
    if (argc > 1 && strcmp(argv[1], "heap") == 0) {
        int *copy = malloc(sizeof r.scores);
        if (copy == NULL)
            return 1;
        memcpy(copy, r.scores, sizeof r.scores);
        printf("search for 99 in a heap copy: %d\n", search(copy, 5, 99));
        free(copy);
        return 0;
    }
    printf("search for 33: %d\n", search(r.scores, 5, 33));
    printf("search for 99: %d\n", search(r.scores, 5, 99));   /* 99 is not a score */
    return 0;
}
