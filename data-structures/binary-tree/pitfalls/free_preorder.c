/* free_preorder.c - freeing a node before its children have been reached.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic free_preorder.c -o free_preorder
 *
 * free(t) comes first, then t->left and t->right are read from the freed
 * block. That is undefined behaviour. With glibc 2.39 it crashes: free()
 * stores a check value in the second 8 bytes of a small block, which is
 * where `left` lives, so the next call follows a garbage pointer.
 */
#define BST_NO_MAIN
#include "../src/bst.c"

void free_preorder(Node *t)
{
    if (t == NULL)
        return;
    free(t);
    free_preorder(t->left);     /* reads freed memory */
    free_preorder(t->right);
}

int main(void)
{
    const int keys[] = {50, 30, 70, 20, 40};
    Node *root = NULL;
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
        bst_insert(&root, keys[i]);
    free_preorder(root);
    printf("freed 5 nodes\n");
    return 0;
}
