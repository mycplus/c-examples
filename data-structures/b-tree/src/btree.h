/* btree.h - a B-tree of 64-bit integer keys and values in C11.
 *
 * A B-tree of minimum degree t keeps between t-1 and 2t-1 keys in every
 * node except the root, and every leaf at the same depth, so a tree of n
 * keys is at most about log_t(n) levels deep. t = 2 gives a 2-3-4 tree;
 * database indexes use nodes with hundreds of keys.
 */
#ifndef MYCPLUS_BTREE_H
#define MYCPLUS_BTREE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef int64_t btree_key;
typedef int64_t btree_value;
typedef struct btree btree;

/* Creates an empty tree of minimum degree t (t >= 2). NULL if t is too
 * small or memory runs out. */
btree *btree_create(int t);
void btree_destroy(btree *tree);

/* Inserts key with value. Returns 1 if the key was added, 0 if it was
 * already present (its value is replaced), -1 if memory ran out (the tree
 * is unchanged). */
int btree_insert(btree *tree, btree_key key, btree_value value);

/* Finds key. Returns true and stores its value in *value (if not NULL). */
bool btree_search(const btree *tree, btree_key key, btree_value *value);

/* Removes key. Returns true if it was present. */
bool btree_remove(btree *tree, btree_key key);

/* Calls visit for every key in [lo, hi] in ascending order; returns the
 * number of keys visited. btree_walk visits every key. */
typedef void (*btree_visit)(btree_key key, btree_value value, void *ctx);
size_t btree_range(const btree *tree, btree_key lo, btree_key hi, btree_visit visit, void *ctx);
void btree_walk(const btree *tree, btree_visit visit, void *ctx);

typedef struct {
    size_t keys;      /* number of keys                                 */
    size_t nodes;     /* number of nodes                                */
    int height;       /* levels: 0 for an empty tree, 1 for a lone root */
    int min_degree;   /* t                                              */
} btree_stats;
btree_stats btree_get_stats(const btree *tree);

/* Number of nodes a search for key reads: the cost a disk-based tree pays
 * in page reads. */
int btree_search_cost(const btree *tree, btree_key key);

/* Checks every B-tree invariant. Returns NULL if the tree is valid, or a
 * description of the first problem found. */
const char *btree_check(const btree *tree);

/* Prints the tree one level per line, each node as [k1 k2 ...]. */
void btree_print(const btree *tree, FILE *out);

#endif
