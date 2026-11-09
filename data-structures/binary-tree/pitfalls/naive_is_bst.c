/* naive_is_bst.c - a BST check that only compares each node with its children.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic naive_is_bst.c -o naive_is_bst
 *
 * Every key in the left subtree must be smaller than the node, not just the
 * left child. The tree built here passes the local check, but 60 sits in
 * the left subtree of 50, so a search for 60 goes right and misses it.
 */
#define BST_NO_MAIN
#include "../src/bst.c"

#include <limits.h>

bool is_bst_local(const Node *t)
{
    if (t == NULL)
        return true;
    if (t->left != NULL && t->left->key >= t->key)
        return false;
    if (t->right != NULL && t->right->key <= t->key)
        return false;
    return is_bst_local(t->left) && is_bst_local(t->right);
}

/* Correct: every key must lie strictly between the bounds its ancestors set.
 * long long bounds let INT_MIN and INT_MAX be keys. */
bool is_bst_range(const Node *t, long long lo, long long hi)
{
    if (t == NULL)
        return true;
    if (t->key <= lo || t->key >= hi)
        return false;
    return is_bst_range(t->left, lo, t->key) && is_bst_range(t->right, t->key, hi);
}

static Node *node(int key, Node *left, Node *right)
{
    Node *n = malloc(sizeof *n);
    if (n == NULL)
        exit(EXIT_FAILURE);
    n->key = key;
    n->left = left;
    n->right = right;
    return n;
}

int main(void)
{
    /*        50
     *       /  \
     *     30    70
     *       \
     *        60      <- larger than 50, but in 50's left subtree */
    Node *root = node(50, node(30, NULL, node(60, NULL, NULL)), node(70, NULL, NULL));
    printf("local check:  %s\n", is_bst_local(root) ? "BST" : "not a BST");
    printf("range check:  %s\n", is_bst_range(root, (long long)INT_MIN - 1, (long long)INT_MAX + 1)
                                     ? "BST" : "not a BST");
    printf("contains 60:  %s\n", bst_contains(root, 60) ? "yes" : "no");
    bst_free(root);
    return 0;
}
