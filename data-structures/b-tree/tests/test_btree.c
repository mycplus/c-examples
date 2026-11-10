/* test_btree.c - unit tests, and a model-based test: thousands of random
 * inserts, updates, searches and removals checked against a plain sorted
 * array, with every B-tree invariant checked after every operation. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "btree.h"

static int failures;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            printf("FAILED line %d: %s\n", __LINE__, #cond);               \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

/* A deterministic 64-bit generator (splitmix64), so runs are repeatable. */
static uint64_t rng_state;
static uint64_t next_random(void)
{
    uint64_t z = (rng_state += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

/* The model: a sorted array of (key, value) pairs. */
typedef struct { btree_key key; btree_value value; } pair;
static pair *model;
static size_t model_n;

static size_t model_find(btree_key key)
{
    size_t lo = 0, hi = model_n;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (model[mid].key < key)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}

typedef struct { const pair *expect; size_t at, n; int ok; } walk_ctx;
static void compare_visit(btree_key key, btree_value value, void *p)
{
    walk_ctx *w = p;
    if (w->at >= w->n || w->expect[w->at].key != key || w->expect[w->at].value != value)
        w->ok = 0;
    w->at++;
}

/* Runs `ops` random operations on keys in [0, key_range) with minimum
 * degree t. check_every: run btree_check after every k operations. */
static int model_ops(btree *tree, int t, int ops, btree_key key_range, int check_every);

static int model_test(int t, int ops, btree_key key_range, int check_every, uint64_t seed)
{
    rng_state = seed;
    btree *tree = btree_create(t);
    model = malloc((size_t)key_range * sizeof *model);
    model_n = 0;
    int ok = tree != NULL && model != NULL && model_ops(tree, t, ops, key_range, check_every);
    btree_destroy(tree);
    free(model);
    return ok;
}

/* The body of model_test; returns 0 at the first disagreement. */
static int model_ops(btree *tree, int t, int ops, btree_key key_range, int check_every)
{
    for (int op = 1; op <= ops; ++op) {
        btree_key key = (btree_key)(next_random() % (uint64_t)key_range);
        size_t i = model_find(key);
        bool present = i < model_n && model[i].key == key;
        switch (next_random() % 4) {
        case 0:
        case 1: {                                   /* insert or update */
            btree_value v = (btree_value)(next_random() >> 1);
            int r = btree_insert(tree, key, v);
            if (r != (present ? 0 : 1)) {
                printf("  t=%d op %d: insert(%lld) returned %d\n", t, op, (long long)key, r);
                return 0;
            }
            if (!present) {
                memmove(model + i + 1, model + i, (model_n - i) * sizeof *model);
                model_n++;
            }
            model[i].key = key;
            model[i].value = v;
            break;
        }
        case 2: {                                   /* remove */
            if (btree_remove(tree, key) != present) {
                printf("  t=%d op %d: remove(%lld) disagreed with the model\n", t, op, (long long)key);
                return 0;
            }
            if (present) {
                memmove(model + i, model + i + 1, (model_n - i - 1) * sizeof *model);
                model_n--;
            }
            break;
        }
        default: {                                  /* search */
            btree_value v = 0;
            bool found = btree_search(tree, key, &v);
            if (found != present || (found && v != model[i].value)) {
                printf("  t=%d op %d: search(%lld) disagreed with the model\n", t, op, (long long)key);
                return 0;
            }
        }
        }
        if (op % check_every == 0 || op == ops) {
            const char *e = btree_check(tree);
            walk_ctx w = {model, 0, model_n, 1};
            btree_walk(tree, compare_visit, &w);
            if (e != NULL || !w.ok || w.at != model_n || btree_get_stats(tree).keys != model_n) {
                printf("  t=%d op %d: %s\n", t, op, e != NULL ? e : "in-order walk differs from the model");
                return 0;
            }
        }
    }
    /* Empty the tree in random order and check it shrinks back to nothing. */
    while (model_n > 0) {
        size_t i = (size_t)(next_random() % model_n);
        if (!btree_remove(tree, model[i].key)) {
            puts("  removing a present key failed while emptying the tree");
            return 0;
        }
        memmove(model + i, model + i + 1, (model_n - i - 1) * sizeof *model);
        model_n--;
        if (model_n % 64 == 0 && btree_check(tree) != NULL) {
            printf("  while emptying: %s\n", btree_check(tree));
            return 0;
        }
    }
    return btree_get_stats(tree).height == 0 && btree_get_stats(tree).nodes == 0;
}

typedef struct { btree_key keys[64]; size_t n; } collect;
static void collect_visit(btree_key key, btree_value value, void *p)
{
    (void)value;
    collect *c = p;
    if (c->n < 64)
        c->keys[c->n++] = key;
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);       /* keep messages if a bug crashes the run */
    /* Creation and the empty tree. */
    CHECK(btree_create(1) == NULL);
    btree *tree = btree_create(2);
    CHECK(tree != NULL);
    CHECK(btree_check(tree) == NULL);
    CHECK(!btree_remove(tree, 5));
    CHECK(!btree_search(tree, 5, NULL));
    CHECK(btree_get_stats(tree).height == 0);

    /* 1..10 into a 2-3-4 tree: three levels, and every key found. */
    for (btree_key k = 1; k <= 10; ++k)
        CHECK(btree_insert(tree, k, k * 100) == 1);
    CHECK(btree_check(tree) == NULL);
    CHECK(btree_get_stats(tree).height == 3);
    btree_value v = 0;
    CHECK(btree_search(tree, 7, &v) && v == 700);
    CHECK(btree_insert(tree, 7, 7777) == 0);             /* update */
    CHECK(btree_search(tree, 7, &v) && v == 7777);
    CHECK(btree_get_stats(tree).keys == 10);

    /* Range queries, including empty and reversed ranges. */
    collect c = {{0}, 0};
    CHECK(btree_range(tree, 3, 6, collect_visit, &c) == 4);
    CHECK(c.n == 4 && c.keys[0] == 3 && c.keys[3] == 6);
    c.n = 0;
    CHECK(btree_range(tree, 11, 20, collect_visit, &c) == 0);
    CHECK(btree_range(tree, 6, 3, collect_visit, &c) == 0);
    CHECK(btree_range(tree, INT64_MIN, INT64_MAX, collect_visit, &c) == 10);

    /* Removing every key returns the tree to empty. */
    for (btree_key k = 10; k >= 1; --k) {
        CHECK(btree_remove(tree, k));
        CHECK(btree_check(tree) == NULL);
    }
    CHECK(btree_get_stats(tree).height == 0 && btree_get_stats(tree).nodes == 0);
    btree_destroy(tree);

    /* Extreme keys. */
    tree = btree_create(3);
    CHECK(btree_insert(tree, INT64_MIN, 1) == 1 && btree_insert(tree, INT64_MAX, 2) == 1);
    CHECK(btree_search(tree, INT64_MIN, &v) && v == 1);
    CHECK(btree_check(tree) == NULL);
    btree_destroy(tree);

    /* Model-based tests across minimum degrees. */
    static const int degrees[] = {2, 3, 4, 5, 8, 32};
    int model_runs = 0, model_ok = 0;
    for (size_t d = 0; d < sizeof degrees / sizeof degrees[0]; ++d) {
        ++model_runs;
        model_ok += model_test(degrees[d], 20000, 2000, 1, 1000 + (uint64_t)d);   /* check every op */
        ++model_runs;
        model_ok += model_test(degrees[d], 200000, 50000, 5000, 2000 + (uint64_t)d);
    }
    printf("model-based runs passed: %d of %d\n", model_ok, model_runs);
    printf("unit checks failed: %d\n", failures);
    return failures == 0 && model_ok == model_runs ? EXIT_SUCCESS : EXIT_FAILURE;
}
