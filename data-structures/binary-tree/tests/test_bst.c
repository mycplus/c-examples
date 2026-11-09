/* test_bst.c - tests for src/bst.c, which it includes so the code tested is
 * the code the article shows.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic -I../src test_bst.c -o test_bst
 */
#define DEEP_TREE_NO_MAIN
#include "../src/deep_tree.c"       /* includes bst.c, adds sorted_chain() */

#include <limits.h>

static int failures;

static void check(bool ok, int line, const char *what)
{
    if (!ok) {
        fprintf(stderr, "FAIL test_bst.c:%d: %s\n", line, what);
        failures++;
    }
}
#define CHECK(cond) check((cond), __LINE__, #cond)

typedef struct {
    int    *keys;
    size_t  len, cap;
} Keys;

static void collect(int key, void *ctx)
{
    Keys *k = ctx;
    if (k->len == k->cap) {
        k->cap = k->cap ? 2 * k->cap : 64;
        int *p = realloc(k->keys, k->cap * sizeof *p);
        if (p == NULL) {
            fprintf(stderr, "out of memory in test\n");
            exit(EXIT_FAILURE);
        }
        k->keys = p;
    }
    k->keys[k->len++] = key;
}

static bool same(const Keys *k, const int *want, size_t n)
{
    if (k->len != n)
        return false;
    for (size_t i = 0; i < n; i++)
        if (k->keys[i] != want[i])
            return false;
    return true;
}

static bool in_range(const Node *t, long long lo, long long hi)
{
    if (t == NULL)
        return true;
    if (t->key <= lo || t->key >= hi)
        return false;
    return in_range(t->left, lo, t->key) && in_range(t->right, t->key, hi);
}

static bool same_shape(const Node *a, const Node *b)
{
    while (a != NULL && b != NULL) {        /* iterative along the right spine */
        if (a->key != b->key || !same_shape(a->left, b->left))
            return false;
        a = a->right;
        b = b->right;
    }
    return a == NULL && b == NULL;
}

static void test_demo_tree(void)
{
    const int keys[] = {50, 30, 70, 20, 40, 60, 80, 35, 45, 65};
    Node *root = NULL;
    for (size_t i = 0; i < 10; i++)
        CHECK(bst_insert(&root, keys[i]) == 1);
    CHECK(bst_insert(&root, 40) == 0);

    const int pre[]  = {50, 30, 20, 40, 35, 45, 70, 60, 65, 80};
    const int in[]   = {20, 30, 35, 40, 45, 50, 60, 65, 70, 80};
    const int post[] = {20, 35, 45, 40, 30, 65, 60, 80, 70, 50};
    const int lvl[]  = {50, 30, 70, 20, 40, 60, 80, 35, 45, 65};
    Keys k = {0};
    bst_preorder(root, collect, &k);      CHECK(same(&k, pre, 10));  k.len = 0;
    bst_inorder(root, collect, &k);       CHECK(same(&k, in, 10));   k.len = 0;
    CHECK(bst_inorder_iter(root, collect, &k)); CHECK(same(&k, in, 10)); k.len = 0;
    bst_postorder(root, collect, &k);     CHECK(same(&k, post, 10)); k.len = 0;
    CHECK(bst_level_order(root, collect, &k)); CHECK(same(&k, lvl, 10)); k.len = 0;
    CHECK(bst_height(root) == 3);
    CHECK(bst_height_iter(root) == 3);
    CHECK(bst_size(root) == 10);

    /* every removal case: leaf, left child only, right child only, two children, root */
    CHECK(bst_remove(&root, 65));          /* leaf */
    CHECK(bst_remove(&root, 40));          /* two children: 45 replaces it */
    CHECK(bst_remove(&root, 45));          /* now has a left child only (35) */
    CHECK(bst_remove(&root, 70));          /* two children: 80 replaces it */
    CHECK(bst_remove(&root, 80));          /* now has a left child only (60) */
    CHECK(bst_remove(&root, 30));          /* right child only (35) */
    CHECK(bst_remove(&root, 50));          /* root with two children */
    CHECK(!bst_remove(&root, 50));
    const int left[] = {20, 35, 60};
    bst_inorder(root, collect, &k);
    CHECK(same(&k, left, 3));
    CHECK(in_range(root, (long long)INT_MIN - 1, (long long)INT_MAX + 1));
    free(k.keys);
    bst_free(root);
}

static void test_empty_and_single(void)
{
    Node *root = NULL;
    Keys k = {0};
    CHECK(bst_height(root) == -1);
    CHECK(bst_height_iter(root) == -1);
    CHECK(bst_size(root) == 0);
    CHECK(!bst_contains(root, 0));
    CHECK(!bst_remove(&root, 0));
    bst_preorder(root, collect, &k);
    bst_inorder(root, collect, &k);
    bst_postorder(root, collect, &k);
    CHECK(bst_inorder_iter(root, collect, &k));
    CHECK(bst_level_order(root, collect, &k));
    CHECK(k.len == 0);
    CHECK(bst_insert(&root, INT_MIN) == 1);
    CHECK(bst_height(root) == 0 && bst_size(root) == 1);
    CHECK(bst_remove(&root, INT_MIN) && root == NULL);
    bst_free(root);                        /* freeing an empty tree is a no-op */
    free(k.keys);
}

/* 200,000 random inserts, removes and searches against a presence table. */
static void test_against_model(void)
{
    enum { RANGE = 2000, OPS = 200000 };
    static bool present[RANGE];
    size_t count = 0;
    unsigned long long x = 2463534242ULL;
    Node *root = NULL;
    Keys k = {0};
    for (int op = 0; op < OPS; op++) {
        x ^= x << 13; x ^= x >> 7; x ^= x << 17;
        size_t i = (size_t)(x % RANGE);
        int key = i == 0 ? INT_MIN : i == RANGE - 1 ? INT_MAX : (int)i * 1000003 - 1000000000;
        switch ((x >> 32) % 3) {
        case 0:
            CHECK(bst_insert(&root, key) == (present[i] ? 0 : 1));
            if (!present[i]) { present[i] = true; count++; }
            break;
        case 1:
            CHECK(bst_remove(&root, key) == present[i]);
            if (present[i]) { present[i] = false; count--; }
            break;
        default:
            CHECK(bst_contains(root, key) == present[i]);
        }
        if (op % 1000 == 0) {
            CHECK(bst_size(root) == count);
            CHECK(in_range(root, (long long)INT_MIN - 1, (long long)INT_MAX + 1));
            CHECK(bst_height(root) == bst_height_iter(root));
            k.len = 0;
            CHECK(bst_inorder_iter(root, collect, &k));
            CHECK(k.len == count);
            for (size_t j = 1; j < k.len; j++)
                CHECK(k.keys[j - 1] < k.keys[j]);
        }
    }
    free(k.keys);
    bst_free(root);
}

/* The directly linked chain is exactly what sorted insertion builds. */
static void test_sorted_chain_matches_insertion(void)
{
    Node *inserted = NULL;
    for (int i = 0; i < 2000; i++)
        CHECK(bst_insert(&inserted, i) == 1);
    Node *chain = sorted_chain(2000);
    CHECK(chain != NULL && same_shape(inserted, chain));
    bst_free(inserted);
    bst_free(chain);
}

/* A million-node chain: the iterative functions and bst_free() do not recurse. */
static void test_deep_chain(void)
{
    const size_t n = 1000000;
    Node *chain = sorted_chain(n);
    CHECK(chain != NULL);
    CHECK(bst_height_iter(chain) == (long)n - 1);
    CHECK(bst_size(chain) == n);
    Keys k = {0};
    CHECK(bst_inorder_iter(chain, collect, &k));
    CHECK(k.len == n && k.keys[0] == 0 && k.keys[n - 1] == (int)n - 1);
    k.len = 0;
    CHECK(bst_level_order(chain, collect, &k));
    CHECK(k.len == n);
    CHECK(bst_remove(&chain, 0) && bst_remove(&chain, (int)n - 1));
    CHECK(bst_size(chain) == n - 2);
    free(k.keys);
    bst_free(chain);
}

int main(void)
{
    test_demo_tree();
    test_empty_and_single();
    test_against_model();
    test_sorted_chain_matches_insertion();
    test_deep_chain();
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("all binary tree tests passed\n");
    return 0;
}
