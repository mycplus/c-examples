/* remove_wrong_key.c - a recursive remove that deletes the wrong key in the
 * two-children case.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic remove_wrong_key.c -o remove_wrong_key
 *
 * After copying the successor's key into the node, the successor has to be
 * removed from the right subtree. Passing the original key there instead
 * finds nothing (that key is gone), so the successor's key is left in the
 * tree twice.
 */
#define BST_NO_MAIN
#include "../src/bst.c"

Node *remove_rec(Node *t, int key)
{
    if (t == NULL)
        return NULL;
    if (key < t->key) {
        t->left = remove_rec(t->left, key);
    } else if (key > t->key) {
        t->right = remove_rec(t->right, key);
    } else if (t->left == NULL || t->right == NULL) {
        Node *child = t->left != NULL ? t->left : t->right;
        free(t);
        return child;
    } else {
        const Node *succ = t->right;
        while (succ->left != NULL)
            succ = succ->left;
        t->key = succ->key;
        t->right = remove_rec(t->right, key);   /* bug: should be succ's key, now t->key */
    }
    return t;
}

static void print_key(int key, void *ctx)
{
    (void)ctx;
    printf(" %d", key);
}

int main(void)
{
    const int keys[] = {50, 30, 70, 20, 40, 60, 80, 35, 45, 65};
    Node *root = NULL;
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
        bst_insert(&root, keys[i]);
    root = remove_rec(root, 30);
    printf("after removing 30:");
    bst_inorder(root, print_key, NULL);
    printf("\nsize %zu, contains 30: %s\n", bst_size(root), bst_contains(root, 30) ? "yes" : "no");
    root = remove_rec(root, 35);
    printf("after removing 35:");
    bst_inorder(root, print_key, NULL);
    printf("\nsize %zu, contains 35: %s\n", bst_size(root), bst_contains(root, 35) ? "yes" : "no");
    bst_free(root);
    return 0;
}
