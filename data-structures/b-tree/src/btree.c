/* btree.c - B-tree insertion, search and deletion in a single pass down the
 * tree, after Bayer and McCreight (1972) and the presentation in Cormen et
 * al., "Introduction to Algorithms". See btree.h for the interface. */
#include "btree.h"

#include <stdlib.h>
#include <string.h>

typedef struct node {
    int n;                  /* keys in use                                  */
    bool leaf;
    btree_key *keys;        /* keys[0..n-1], ascending; room for 2t-1       */
    btree_value *vals;      /* vals[i] belongs to keys[i]                   */
    struct node **child;    /* child[0..n] if not a leaf; room for 2t       */
} node;

struct btree {
    node *root;             /* NULL when the tree is empty */
    int t;                  /* minimum degree              */
    size_t count;
};

/* One allocation per node: the header, then the three arrays. */
static node *node_new(int t, bool leaf)
{
    size_t max_keys = (size_t)(2 * t - 1);
    node *x = malloc(sizeof *x + max_keys * (sizeof(btree_key) + sizeof(btree_value)) +
                     (max_keys + 1) * sizeof(node *));
    if (x == NULL)
        return NULL;
    x->n = 0;
    x->leaf = leaf;
    x->keys = (btree_key *)(x + 1);
    x->vals = (btree_value *)(x->keys + max_keys);
    x->child = (node **)(x->vals + max_keys);
    return x;
}

static void node_free(node *x)
{
    if (x == NULL)
        return;
    if (!x->leaf)
        for (int i = 0; i <= x->n; ++i)
            node_free(x->child[i]);
    free(x);
}

/* The first position i with keys[i] >= key (binary search). */
static int lower_bound(const node *x, btree_key key)
{
    int lo = 0, hi = x->n;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (x->keys[mid] < key)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}

btree *btree_create(int t)
{
    if (t < 2)
        return NULL;
    btree *tree = malloc(sizeof *tree);
    if (tree != NULL) {
        tree->root = NULL;
        tree->t = t;
        tree->count = 0;
    }
    return tree;
}

void btree_destroy(btree *tree)
{
    if (tree != NULL) {
        node_free(tree->root);
        free(tree);
    }
}

bool btree_search(const btree *tree, btree_key key, btree_value *value)
{
    const node *x = tree->root;
    while (x != NULL) {
        int i = lower_bound(x, key);
        if (i < x->n && x->keys[i] == key) {
            if (value != NULL)
                *value = x->vals[i];
            return true;
        }
        x = x->leaf ? NULL : x->child[i];
    }
    return false;
}

int btree_search_cost(const btree *tree, btree_key key)
{
    int reads = 0;
    const node *x = tree->root;
    while (x != NULL) {
        ++reads;
        int i = lower_bound(x, key);
        if (i < x->n && x->keys[i] == key)
            break;
        x = x->leaf ? NULL : x->child[i];
    }
    return reads;
}

/* ---- Insertion ----------------------------------------------------------- */

/* Splits the full child y = x->child[i] around its median key, which moves
 * up into x. y keeps the t-1 smaller keys; a new node z takes the t-1
 * larger ones. x must not be full. */
static bool split_child(node *x, int i, int t)
{
    node *y = x->child[i];
    node *z = node_new(t, y->leaf);
    if (z == NULL)
        return false;
    z->n = t - 1;
    memcpy(z->keys, y->keys + t, (size_t)(t - 1) * sizeof *z->keys);
    memcpy(z->vals, y->vals + t, (size_t)(t - 1) * sizeof *z->vals);
    if (!y->leaf)
        memcpy(z->child, y->child + t, (size_t)t * sizeof *z->child);
    y->n = t - 1;

    /* Open a gap at position i in x for the median and at i+1 for z. */
    memmove(x->child + i + 2, x->child + i + 1, (size_t)(x->n - i) * sizeof *x->child);
    memmove(x->keys + i + 1, x->keys + i, (size_t)(x->n - i) * sizeof *x->keys);
    memmove(x->vals + i + 1, x->vals + i, (size_t)(x->n - i) * sizeof *x->vals);
    x->child[i + 1] = z;
    x->keys[i] = y->keys[t - 1];
    x->vals[i] = y->vals[t - 1];
    x->n++;
    return true;
}

int btree_insert(btree *tree, btree_key key, btree_value value)
{
    int t = tree->t;
    if (tree->root == NULL && (tree->root = node_new(t, true)) == NULL)
        return -1;

    /* A full root splits first: this is the only way the tree grows taller. */
    if (tree->root->n == 2 * t - 1) {
        node *s = node_new(t, false);
        if (s == NULL)
            return -1;
        s->child[0] = tree->root;
        if (!split_child(s, 0, t)) {
            free(s);
            return -1;
        }
        tree->root = s;
    }

    /* Walk down, splitting every full child before entering it, so the
     * leaf at the bottom always has room. A key already in the tree is
     * met on the way and only its value changes. */
    node *x = tree->root;
    for (;;) {
        int i = lower_bound(x, key);
        if (i < x->n && x->keys[i] == key) {
            x->vals[i] = value;
            return 0;
        }
        if (x->leaf) {
            memmove(x->keys + i + 1, x->keys + i, (size_t)(x->n - i) * sizeof *x->keys);
            memmove(x->vals + i + 1, x->vals + i, (size_t)(x->n - i) * sizeof *x->vals);
            x->keys[i] = key;
            x->vals[i] = value;
            x->n++;
            tree->count++;
            return 1;
        }
        if (x->child[i]->n == 2 * t - 1) {
            if (!split_child(x, i, t))
                return -1;
            if (key == x->keys[i]) {            /* the median that moved up */
                x->vals[i] = value;
                return 0;
            }
            if (key > x->keys[i])
                ++i;
        }
        x = x->child[i];
    }
}

/* ---- Deletion ------------------------------------------------------------ */

/* Removes keys[i] and child[i+1] from x (used after a merge). */
static void remove_from_node(node *x, int i)
{
    memmove(x->keys + i, x->keys + i + 1, (size_t)(x->n - i - 1) * sizeof *x->keys);
    memmove(x->vals + i, x->vals + i + 1, (size_t)(x->n - i - 1) * sizeof *x->vals);
    if (!x->leaf)
        memmove(x->child + i + 1, x->child + i + 2, (size_t)(x->n - i - 1) * sizeof *x->child);
    x->n--;
}

/* Merges child[i+1] and the separating key keys[i] into child[i]. Both
 * children have t-1 keys, so the result has exactly 2t-1. */
static void merge_children(node *x, int i)
{
    node *y = x->child[i], *z = x->child[i + 1];
    y->keys[y->n] = x->keys[i];
    y->vals[y->n] = x->vals[i];
    memcpy(y->keys + y->n + 1, z->keys, (size_t)z->n * sizeof *z->keys);
    memcpy(y->vals + y->n + 1, z->vals, (size_t)z->n * sizeof *z->vals);
    if (!y->leaf)
        memcpy(y->child + y->n + 1, z->child, (size_t)(z->n + 1) * sizeof *z->child);
    y->n += z->n + 1;
    remove_from_node(x, i);
    free(z);
}

/* child[i] has t-1 keys: move one key down into it from x, and one key up
 * into x from its left sibling (a rotation). */
static void borrow_from_left(node *x, int i)
{
    node *c = x->child[i], *s = x->child[i - 1];
    memmove(c->keys + 1, c->keys, (size_t)c->n * sizeof *c->keys);
    memmove(c->vals + 1, c->vals, (size_t)c->n * sizeof *c->vals);
    if (!c->leaf)
        memmove(c->child + 1, c->child, (size_t)(c->n + 1) * sizeof *c->child);
    c->keys[0] = x->keys[i - 1];
    c->vals[0] = x->vals[i - 1];
    if (!c->leaf)
        c->child[0] = s->child[s->n];
    x->keys[i - 1] = s->keys[s->n - 1];
    x->vals[i - 1] = s->vals[s->n - 1];
    c->n++;
    s->n--;
}

/* The mirror image: one key from the right sibling via x. */
static void borrow_from_right(node *x, int i)
{
    node *c = x->child[i], *s = x->child[i + 1];
    c->keys[c->n] = x->keys[i];
    c->vals[c->n] = x->vals[i];
    if (!c->leaf)
        c->child[c->n + 1] = s->child[0];
    c->n++;
    x->keys[i] = s->keys[0];
    x->vals[i] = s->vals[0];
    memmove(s->keys, s->keys + 1, (size_t)(s->n - 1) * sizeof *s->keys);
    memmove(s->vals, s->vals + 1, (size_t)(s->n - 1) * sizeof *s->vals);
    if (!s->leaf)
        memmove(s->child, s->child + 1, (size_t)s->n * sizeof *s->child);
    s->n--;
}

/* Makes sure x->child[i] has at least t keys before the deletion enters
 * it; returns the index of the child to enter (it changes after merging
 * with the left sibling). */
static int fill_child(node *x, int i, int t)
{
    if (x->child[i]->n >= t)
        return i;
    if (i > 0 && x->child[i - 1]->n >= t) {
        borrow_from_left(x, i);
    } else if (i < x->n && x->child[i + 1]->n >= t) {
        borrow_from_right(x, i);
    } else if (i < x->n) {
        merge_children(x, i);
    } else {
        merge_children(x, i - 1);
        --i;
    }
    return i;
}

/* Removes key from the subtree rooted at x, which has at least t keys
 * (or is the root). */
static bool remove_key(node *x, btree_key key, int t)
{
    for (;;) {
        int i = lower_bound(x, key);
        bool here = i < x->n && x->keys[i] == key;

        if (here && x->leaf) {                        /* case 1 */
            remove_from_node(x, i);
            return true;
        }
        if (here) {                                   /* case 2: internal node */
            node *y = x->child[i], *z = x->child[i + 1];
            if (y->n >= t) {                          /* 2a: use the predecessor */
                node *p = y;
                while (!p->leaf)
                    p = p->child[p->n];
                x->keys[i] = p->keys[p->n - 1];
                x->vals[i] = p->vals[p->n - 1];
                key = x->keys[i];
                x = y;
            } else if (z->n >= t) {                   /* 2b: use the successor */
                node *s = z;
                while (!s->leaf)
                    s = s->child[0];
                x->keys[i] = s->keys[0];
                x->vals[i] = s->vals[0];
                key = x->keys[i];
                x = z;
            } else {                                  /* 2c: merge, then go on */
                merge_children(x, i);
                x = y;
            }
            continue;
        }
        if (x->leaf)
            return false;                             /* not in the tree */
        x = x->child[fill_child(x, i, t)];            /* case 3 */
    }
}

bool btree_remove(btree *tree, btree_key key)
{
    if (tree->root == NULL)
        return false;
    /* One pass down; nodes on the path may be rebalanced even if the key
     * turns out to be absent, which leaves a valid tree. */
    bool removed = remove_key(tree->root, key, tree->t);
    if (removed)
        tree->count--;

    /* An empty root: the tree shrinks by one level, or becomes empty. */
    node *r = tree->root;
    if (r->n == 0) {
        tree->root = r->leaf ? NULL : r->child[0];
        free(r);
    }
    return removed;
}

/* ---- Traversal, statistics, checking, printing --------------------------- */

static size_t range_node(const node *x, btree_key lo, btree_key hi, btree_visit visit, void *ctx)
{
    size_t visited = 0;
    int i = lower_bound(x, lo);
    for (; i <= x->n; ++i) {
        if (!x->leaf)
            visited += range_node(x->child[i], lo, hi, visit, ctx);
        if (i == x->n || x->keys[i] > hi)
            break;
        visit(x->keys[i], x->vals[i], ctx);
        ++visited;
    }
    return visited;
}

size_t btree_range(const btree *tree, btree_key lo, btree_key hi, btree_visit visit, void *ctx)
{
    return tree->root == NULL || lo > hi ? 0 : range_node(tree->root, lo, hi, visit, ctx);
}

void btree_walk(const btree *tree, btree_visit visit, void *ctx)
{
    if (tree->root != NULL)
        range_node(tree->root, INT64_MIN, INT64_MAX, visit, ctx);
}

static void stats_node(const node *x, btree_stats *s)
{
    s->nodes++;
    if (!x->leaf)
        for (int i = 0; i <= x->n; ++i)
            stats_node(x->child[i], s);
}

btree_stats btree_get_stats(const btree *tree)
{
    btree_stats s = {tree->count, 0, 0, tree->t};
    for (const node *x = tree->root; x != NULL; x = x->leaf ? NULL : x->child[0])
        s.height++;
    if (tree->root != NULL)
        stats_node(tree->root, &s);
    return s;
}

/* Checks the subtree at x: key counts, order, bounds (lo, hi) given by the
 * ancestors' keys, and leaf depth. Counts keys into *keys. */
static const char *check_node(const node *x, int t, bool is_root, bool has_lo, btree_key lo,
                              bool has_hi, btree_key hi, int depth, int *leaf_depth, size_t *keys)
{
    if (x->n > 2 * t - 1)
        return "a node has more than 2t-1 keys";
    if (!is_root && x->n < t - 1)
        return "a node other than the root has fewer than t-1 keys";
    if (is_root && x->n < 1)
        return "the root of a non-empty tree has no keys";
    for (int i = 0; i < x->n; ++i) {
        if (i > 0 && x->keys[i - 1] >= x->keys[i])
            return "keys in a node are not in ascending order";
        if ((has_lo && x->keys[i] <= lo) || (has_hi && x->keys[i] >= hi))
            return "a key is outside the range its ancestors allow";
    }
    *keys += (size_t)x->n;
    if (x->leaf) {
        if (*leaf_depth < 0)
            *leaf_depth = depth;
        return *leaf_depth == depth ? NULL : "leaves are at different depths";
    }
    for (int i = 0; i <= x->n; ++i) {
        if (x->child[i] == NULL)
            return "an internal node is missing a child";
        const char *e = check_node(x->child[i], t, false, i > 0 || has_lo, i > 0 ? x->keys[i - 1] : lo,
                                   i < x->n || has_hi, i < x->n ? x->keys[i] : hi, depth + 1,
                                   leaf_depth, keys);
        if (e != NULL)
            return e;
    }
    return NULL;
}

const char *btree_check(const btree *tree)
{
    if (tree->root == NULL)
        return tree->count == 0 ? NULL : "the tree is empty but its count is not zero";
    int leaf_depth = -1;
    size_t keys = 0;
    const char *e = check_node(tree->root, tree->t, true, false, 0, false, 0, 0, &leaf_depth, &keys);
    if (e == NULL && keys != tree->count)
        e = "the number of keys in the nodes does not match the count";
    return e;
}

void btree_print(const btree *tree, FILE *out)
{
    if (tree->root == NULL) {
        fputs("(empty)\n", out);
        return;
    }
    /* Breadth-first, one level per line, using a queue of node pointers. */
    btree_stats s = btree_get_stats(tree);
    const node **queue = malloc(s.nodes * sizeof *queue);
    if (queue == NULL)
        return;
    size_t head = 0, tail = 0;
    queue[tail++] = tree->root;
    while (head < tail) {
        size_t level_end = tail;
        for (; head < level_end; ++head) {
            const node *x = queue[head];
            fputc('[', out);
            for (int i = 0; i < x->n; ++i)
                fprintf(out, i ? " %lld" : "%lld", (long long)x->keys[i]);
            fputs(head + 1 < level_end ? "] " : "]\n", out);
            if (!x->leaf)
                for (int i = 0; i <= x->n; ++i)
                    queue[tail++] = x->child[i];
        }
    }
    free(queue);
}
