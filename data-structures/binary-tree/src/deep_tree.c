/* deep_tree.c - recursive and iterative height on a tree as deep as it is big.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic -O2 deep_tree.c -o deep_tree
 * Usage: deep_tree N [recursive|iterative]
 *
 * Inserting the keys 0..N-1 in sorted order gives a tree in which every node
 * has only a right child: a chain of height N - 1. Building it through
 * bst_insert() costs N(N-1)/2 steps, about 5 * 10^11 for a million keys, so
 * this program links the same chain directly in O(N). The test suite checks
 * that this produces exactly the tree sorted insertion produces.
 */
#define BST_NO_MAIN
#include "bst.c"

#include <limits.h>
#include <string.h>

/* The tree bst_insert() builds from the keys 0..n-1 inserted in order. */
Node *sorted_chain(size_t n)
{
    Node *root = NULL, **link = &root;
    for (size_t i = 0; i < n; i++) {
        Node *node = malloc(sizeof *node);
        if (node == NULL) {
            bst_free(root);
            return NULL;
        }
        node->key = (int)i;
        node->left = node->right = NULL;
        *link = node;
        link = &node->right;
    }
    return root;
}

#ifndef DEEP_TREE_NO_MAIN
int main(int argc, char **argv)
{
    if (argc != 3 || (strcmp(argv[2], "recursive") != 0 && strcmp(argv[2], "iterative") != 0)) {
        fprintf(stderr, "usage: %s N recursive|iterative\n", argv[0]);
        return EXIT_FAILURE;
    }
    char *end;
    unsigned long long n = strtoull(argv[1], &end, 10);
    if (*end != '\0' || n == 0 || n > (unsigned long long)INT_MAX) {
        fprintf(stderr, "N must be between 1 and %d\n", INT_MAX);
        return EXIT_FAILURE;
    }
    Node *root = sorted_chain((size_t)n);
    if (root == NULL) {
        fprintf(stderr, "out of memory\n");
        return EXIT_FAILURE;
    }
    printf("n = %llu, ", n);
    fflush(stdout);                         /* so the line survives a crash */
    if (strcmp(argv[2], "recursive") == 0)
        printf("recursive height = %d\n", bst_height(root));
    else
        printf("iterative height = %ld\n", bst_height_iter(root));
    bst_free(root);
    return 0;
}
#endif
