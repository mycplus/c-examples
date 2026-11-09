#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int *data;
    size_t len;
    size_t cap;
} IntList;

static bool list_push(IntList *l, int value)
{
    if (l->len == l->cap) {
        if (l->cap > SIZE_MAX / 2 / sizeof *l->data)
            return false;                    /* doubling would overflow size_t */
        size_t new_cap = l->cap ? l->cap * 2 : 4;
        /* Keep the result in a temporary: on failure l->data is still valid. */
        int *bigger = realloc(l->data, new_cap * sizeof *bigger);
        if (bigger == NULL)
            return false;
        l->data = bigger;
        l->cap = new_cap;
        printf("  grew to capacity %zu\n", new_cap);
    }
    l->data[l->len++] = value;
    return true;
}

static void list_free(IntList *l)
{
    free(l->data);
    l->data = NULL;
    l->len = l->cap = 0;
}

int main(void)
{
    const char *text = "17 4 23 8 15 42 16 9 31 5";
    IntList list = {0};

    /* strtol advances `end` past each number it reads. */
    const char *cursor = text;
    char *end;
    for (long v = strtol(cursor, &end, 10); end != cursor; v = strtol(cursor, &end, 10)) {
        cursor = end;
        if (!list_push(&list, (int)v)) {
            fprintf(stderr, "out of memory\n");
            list_free(&list);
            return 1;
        }
    }
    printf("read %zu numbers\n", list.len);

    /* calloc gives zeroed memory: a histogram of the values, bucketed by tens. */
    int *buckets = calloc(5, sizeof *buckets);
    if (buckets == NULL) {
        list_free(&list);
        return 1;
    }
    for (size_t i = 0; i < list.len; i++)
        buckets[list.data[i] / 10]++;
    for (int b = 0; b < 5; b++)
        printf("%2d-%2d: %d\n", b * 10, b * 10 + 9, buckets[b]);

    free(buckets);
    list_free(&list);
    return 0;
}
