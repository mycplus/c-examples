/* btree_demo.c - builds a small B-tree of minimum degree 2 (a 2-3-4 tree),
 * prints it level by level, then searches, removes keys and runs a range
 * query, checking every invariant after each change.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic btree_demo.c btree.c -o btree_demo */
#include <stdio.h>
#include <stdlib.h>

#include "btree.h"

static void print_key(btree_key key, btree_value value, void *ctx)
{
    (void)value;
    (void)ctx;
    printf(" %lld", (long long)key);
}

static void show(const btree *tree, const char *title)
{
    btree_stats s = btree_get_stats(tree);
    const char *problem = btree_check(tree);
    printf("%s: %zu keys, %zu nodes, height %d, %s\n", title, s.keys, s.nodes, s.height,
           problem == NULL ? "valid" : problem);
    btree_print(tree, stdout);
}

int main(void)
{
    static const btree_key keys[] = {50, 20, 80, 10, 30, 60, 90, 5, 15,
                                     25, 35, 55, 65, 85, 95, 1, 12, 33};
    btree *tree = btree_create(2);
    if (tree == NULL)
        return EXIT_FAILURE;
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; ++i)
        if (btree_insert(tree, keys[i], keys[i] * 10) < 0)
            return EXIT_FAILURE;
    show(tree, "After 18 inserts");

    btree_value v = 0;
    if (btree_search(tree, 65, &v))
        printf("search 65: found, value %lld, %d nodes read\n", (long long)v,
               btree_search_cost(tree, 65));
    printf("search 66: %s\n", btree_search(tree, 66, NULL) ? "found" : "not found");

    btree_remove(tree, 20);     /* a key in an internal node */
    btree_remove(tree, 1);      /* a key in a leaf */
    btree_remove(tree, 5);
    show(tree, "After removing 20, 1 and 5");

    printf("keys in [30, 60]:");
    size_t n = btree_range(tree, 30, 60, print_key, NULL);
    printf("  (%zu keys)\n", n);

    btree_destroy(tree);
    return 0;
}
