/* insert_by_value.c - an insert that receives the root pointer by value.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic insert_by_value.c -o insert_by_value
 *
 * The new node is stored in the function's own copy of the pointer, which
 * disappears when the function returns. The caller's tree never changes,
 * and every node allocated is leaked.
 */
#define BST_NO_MAIN
#include "../src/bst.c"

void insert_by_value(Node *root, int key)
{
    if (root == NULL) {
        root = malloc(sizeof *root);    /* assigns the parameter, not the caller's pointer */
        if (root == NULL)
            return;
        root->key = key;
        root->left = root->right = NULL;
    } else if (key < root->key) {
        insert_by_value(root->left, key);
    } else if (key > root->key) {
        insert_by_value(root->right, key);
    }
}

int main(void)
{
    Node *root = NULL;
    const int keys[] = {50, 30, 70, 20, 40};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
        insert_by_value(root, keys[i]);
    printf("after 5 inserts: size %zu, root %s\n", bst_size(root), root ? "set" : "NULL");
    fflush(stdout);     /* LeakSanitizer ends the process before stdio flushes */
    bst_free(root);
    return 0;
}
