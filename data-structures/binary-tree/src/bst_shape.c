/* bst_shape.c - how the insertion order shapes a binary search tree.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic -O2 bst_shape.c -o bst_shape
 *
 * Inserts the keys 0..n-1 in random order (a fixed-seed shuffle, so the run
 * is reproducible) and in sorted order, and reports the height and the mean
 * depth of a node. For random order, the mean depth is compared with the
 * expected value for a random binary search tree, 2(1 + 1/n)H(n) - 4.
 */
#define BST_NO_MAIN
#include "bst.c"

#include <time.h>

static unsigned long long rng = 88172645463325252ULL;   /* xorshift64 */

static unsigned long long next_rand(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return rng;
}

static void shuffle(int *a, size_t n)
{
    for (size_t i = n; i > 1; i--) {
        size_t j = (size_t)(next_rand() % i);
        int t = a[i - 1];
        a[i - 1] = a[j];
        a[j] = t;
    }
}

/* Sum of the depths of all nodes (depth of the root = 0), with a queue. */
static double total_depth(const Node *t)
{
    typedef struct { const Node *n; long d; } Item;
    size_t cap = 1024, head = 0, tail = 0;
    Item *q = malloc(cap * sizeof *q);
    double sum = 0;
    if (q == NULL)
        return -1;
    if (t != NULL)
        q[tail++] = (Item){t, 0};
    while (head < tail) {
        Item it = q[head++];
        sum += (double)it.d;
        if (tail + 2 > cap) {
            if (head > 0) {                  /* slide the live part to the front */
                for (size_t i = head; i < tail; i++)
                    q[i - head] = q[i];
                tail -= head;
                head = 0;
            }
            if (tail + 2 > cap) {
                Item *p = realloc(q, 2 * cap * sizeof *q);
                if (p == NULL) {
                    free(q);
                    return -1;
                }
                q = p;
                cap *= 2;
            }
        }
        if (it.n->left != NULL)
            q[tail++] = (Item){it.n->left, it.d + 1};
        if (it.n->right != NULL)
            q[tail++] = (Item){it.n->right, it.d + 1};
    }
    free(q);
    return sum;
}

static Node *build(const int *keys, size_t n)
{
    Node *root = NULL;
    for (size_t i = 0; i < n; i++)
        if (bst_insert(&root, keys[i]) < 0) {
            fprintf(stderr, "out of memory\n");
            exit(EXIT_FAILURE);
        }
    return root;
}

static double ms_since(struct timespec t0)
{
    struct timespec t1;
    timespec_get(&t1, TIME_UTC);
    return (double)(t1.tv_sec - t0.tv_sec) * 1e3 + (double)(t1.tv_nsec - t0.tv_nsec) / 1e6;
}

int main(void)
{
    static const struct { size_t n; int trials; } runs[] = {
        {1000, 1000}, {10000, 200}, {100000, 50}, {1000000, 10},
    };

    printf("random insertion order (keys 0..n-1, fixed-seed shuffles)\n");
    printf("%9s %7s %13s %11s %15s %10s\n",
           "n", "trials", "height range", "mean height", "mean depth", "predicted");
    for (size_t r = 0; r < sizeof runs / sizeof runs[0]; r++) {
        size_t n = runs[r].n;
        int *keys = malloc(n * sizeof *keys);
        if (keys == NULL)
            return EXIT_FAILURE;
        long hmin = 0, hmax = 0;
        double hsum = 0, dsum = 0;
        for (int t = 0; t < runs[r].trials; t++) {
            for (size_t i = 0; i < n; i++)
                keys[i] = (int)i;
            shuffle(keys, n);
            Node *root = build(keys, n);
            long h = bst_height_iter(root);
            hmin = t == 0 || h < hmin ? h : hmin;
            hmax = t == 0 || h > hmax ? h : hmax;
            hsum += (double)h;
            dsum += total_depth(root) / (double)n;
            bst_free(root);
        }
        double harmonic = 0;
        for (size_t k = 1; k <= n; k++)
            harmonic += 1.0 / (double)k;
        double predicted = 2.0 * (1.0 + 1.0 / (double)n) * harmonic - 4.0;
        char range[32];
        snprintf(range, sizeof range, "%ld-%ld", hmin, hmax);
        printf("%9zu %7d %13s %11.1f %15.3f %10.3f\n", n, runs[r].trials, range,
               hsum / runs[r].trials, dsum / runs[r].trials, predicted);
        free(keys);
    }

    printf("\nsorted insertion order\n");
    printf("%9s %9s %15s %18s %12s\n", "n", "height", "mean depth", "insert steps", "time (ms)");
    static const size_t sorted_n[] = {1000, 10000, 20000};
    for (size_t r = 0; r < sizeof sorted_n / sizeof sorted_n[0]; r++) {
        size_t n = sorted_n[r];
        int *keys = malloc(n * sizeof *keys);
        if (keys == NULL)
            return EXIT_FAILURE;
        for (size_t i = 0; i < n; i++)
            keys[i] = (int)i;
        struct timespec t0;
        timespec_get(&t0, TIME_UTC);
        Node *root = build(keys, n);
        double ms = ms_since(t0);
        /* Steps = nodes passed on the way down = sum of the final depths. */
        double depth = total_depth(root);
        printf("%9zu %9ld %15.1f %18.0f %12.1f\n", n, bst_height_iter(root),
               depth / (double)n, depth, ms);
        bst_free(root);
        free(keys);
    }

    printf("\nsame n = 20000, random order for comparison\n");
    {
        size_t n = 20000;
        int *keys = malloc(n * sizeof *keys);
        if (keys == NULL)
            return EXIT_FAILURE;
        for (size_t i = 0; i < n; i++)
            keys[i] = (int)i;
        shuffle(keys, n);
        struct timespec t0;
        timespec_get(&t0, TIME_UTC);
        Node *root = build(keys, n);
        double ms = ms_since(t0);
        double depth = total_depth(root);
        printf("%9zu %9ld %15.1f %18.0f %12.1f\n", n, bst_height_iter(root),
               depth / (double)n, depth, ms);
        bst_free(root);
        free(keys);
    }
    return 0;
}
