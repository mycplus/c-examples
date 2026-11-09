/* bst.c - a binary search tree of int keys, with the four traversals.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic bst.c -o bst
 *
 * Keys are unique: inserting a key that is already present changes nothing.
 * Insert, search and remove walk down the tree with a loop, so they use no
 * recursion; their cost is the height of the tree. The recursive traversals
 * use one stack frame per level, so a tree as deep as its size (sorted
 * insertion) can overflow the call stack; bst_inorder_iter() and
 * bst_level_order() do not recurse.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int          key;
    struct Node *left;    /* keys smaller than key */
    struct Node *right;   /* keys larger than key  */
} Node;

typedef void (*Visit)(int key, void *ctx);

/* Returns 1 if inserted, 0 if the key was already present, -1 if malloc failed. */
int bst_insert(Node **root, int key)
{
    Node **link = root;                     /* the pointer that will hold the node */
    while (*link != NULL) {
        if (key < (*link)->key)
            link = &(*link)->left;
        else if (key > (*link)->key)
            link = &(*link)->right;
        else
            return 0;
    }
    Node *n = malloc(sizeof *n);
    if (n == NULL)
        return -1;
    n->key = key;
    n->left = n->right = NULL;
    *link = n;                              /* writes into the parent, or into *root */
    return 1;
}

bool bst_contains(const Node *root, int key)
{
    while (root != NULL) {
        if (key < root->key)
            root = root->left;
        else if (key > root->key)
            root = root->right;
        else
            return true;
    }
    return false;
}

/* Returns true if the key was found and removed. */
bool bst_remove(Node **root, int key)
{
    Node **link = root;
    while (*link != NULL && (*link)->key != key)
        link = key < (*link)->key ? &(*link)->left : &(*link)->right;
    Node *n = *link;
    if (n == NULL)
        return false;

    if (n->left == NULL) {
        *link = n->right;                   /* zero children or right child only */
    } else if (n->right == NULL) {
        *link = n->left;                    /* left child only */
    } else {
        /* Two children: the successor is the smallest key in the right subtree.
         * It has no left child, so it can be unlinked by replacing it with its
         * right child. Its key then takes the removed key's place. */
        Node **succ_link = &n->right;
        while ((*succ_link)->left != NULL)
            succ_link = &(*succ_link)->left;
        Node *succ = *succ_link;
        *succ_link = succ->right;
        n->key = succ->key;
        n = succ;                           /* the node to free */
    }
    free(n);
    return true;
}

/* Recursive depth-first traversals: one stack frame per level. */
void bst_preorder(const Node *t, Visit visit, void *ctx)
{
    if (t == NULL)
        return;
    visit(t->key, ctx);
    bst_preorder(t->left, visit, ctx);
    bst_preorder(t->right, visit, ctx);
}

void bst_inorder(const Node *t, Visit visit, void *ctx)
{
    if (t == NULL)
        return;
    bst_inorder(t->left, visit, ctx);
    visit(t->key, ctx);
    bst_inorder(t->right, visit, ctx);
}

void bst_postorder(const Node *t, Visit visit, void *ctx)
{
    if (t == NULL)
        return;
    bst_postorder(t->left, visit, ctx);
    bst_postorder(t->right, visit, ctx);
    visit(t->key, ctx);
}

/* A growable array of node pointers, used as a stack or a queue. */
typedef struct {
    const Node **items;
    size_t       len, cap;
} NodeVec;

static bool vec_push(NodeVec *v, const Node *n)
{
    if (v->len == v->cap) {
        size_t cap = v->cap ? v->cap * 2 : 16;
        if (cap > SIZE_MAX / sizeof *v->items)
            return false;
        const Node **p = realloc(v->items, cap * sizeof *v->items);
        if (p == NULL)
            return false;
        v->items = p;
        v->cap = cap;
    }
    v->items[v->len++] = n;
    return true;
}

/* In-order with an explicit stack: the stack grows on the heap, not the call
 * stack. Returns false if it could not allocate; some keys may have been
 * visited by then. */
bool bst_inorder_iter(const Node *t, Visit visit, void *ctx)
{
    NodeVec stack = {0};
    bool ok = true;
    while (ok && (t != NULL || stack.len > 0)) {
        if (t != NULL) {
            ok = vec_push(&stack, t);       /* go left, remembering the path */
            t = t->left;
        } else {
            t = stack.items[--stack.len];
            visit(t->key, ctx);
            t = t->right;
        }
    }
    free(stack.items);
    return ok;
}

/* Breadth-first: level by level, left to right, with a queue. */
bool bst_level_order(const Node *t, Visit visit, void *ctx)
{
    NodeVec queue = {0};
    size_t head = 0;
    bool ok = t == NULL || vec_push(&queue, t);
    while (ok && head < queue.len) {
        const Node *n = queue.items[head++];
        visit(n->key, ctx);
        if (n->left != NULL)
            ok = vec_push(&queue, n->left);
        if (ok && n->right != NULL)
            ok = vec_push(&queue, n->right);
    }
    free(queue.items);
    return ok;
}

/* Height in edges: -1 for an empty tree, 0 for a single node.
 * Recursive: calls nest height + 2 deep, counting the empty children. */
int bst_height(const Node *t)
{
    if (t == NULL)
        return -1;
    int hl = bst_height(t->left);
    int hr = bst_height(t->right);
    return 1 + (hl > hr ? hl : hr);
}

/* The same height without recursion: count the levels of a level-order walk.
 * Returns -2 if it could not allocate. */
long bst_height_iter(const Node *t)
{
    NodeVec level = {0}, next = {0};
    long height = -1;
    bool ok = t == NULL || vec_push(&level, t);
    while (ok && level.len > 0) {
        height++;
        next.len = 0;
        for (size_t i = 0; ok && i < level.len; i++) {
            if (level.items[i]->left != NULL)
                ok = vec_push(&next, level.items[i]->left);
            if (ok && level.items[i]->right != NULL)
                ok = vec_push(&next, level.items[i]->right);
        }
        NodeVec tmp = level;                /* the next level becomes the current one */
        level = next;
        next = tmp;
    }
    free(level.items);
    free(next.items);
    return ok ? height : -2;
}

size_t bst_size(const Node *t)
{
    size_t n = 0;                           /* preorder with an explicit stack */
    NodeVec stack = {0};
    while (t != NULL) {
        n++;
        if (t->right != NULL && !vec_push(&stack, t->right))
            break;                          /* out of memory: partial count */
        t = t->left != NULL ? t->left : (stack.len > 0 ? stack.items[--stack.len] : NULL);
    }
    free(stack.items);
    return n;
}

/* Frees every node without recursion and without extra memory: rotate each
 * left child up until the current node has none, then free it and move right. */
void bst_free(Node *t)
{
    while (t != NULL) {
        if (t->left != NULL) {
            Node *l = t->left;
            t->left = l->right;
            l->right = t;
            t = l;
        } else {
            Node *r = t->right;
            free(t);
            t = r;
        }
    }
}

#ifndef BST_NO_MAIN
static void print_key(int key, void *ctx)
{
    int *count = ctx;
    printf("%s%d", (*count)++ ? " " : "", key);
}

/* Print a label, then the keys a traversal visits, then a newline. */
static void print_rec(const char *label, void (*walk)(const Node *, Visit, void *), const Node *t)
{
    int count = 0;
    printf("%-13s", label);
    walk(t, print_key, &count);
    printf("\n");
}

static void print_iter(const char *label, bool (*walk)(const Node *, Visit, void *), const Node *t)
{
    int count = 0;
    printf("%-13s", label);
    if (!walk(t, print_key, &count))
        printf(" (out of memory)");
    printf("\n");
}

int main(void)
{
    const int keys[] = {50, 30, 70, 20, 40, 60, 80, 35, 45, 65};
    Node *root = NULL;
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
        if (bst_insert(&root, keys[i]) < 0) {
            fprintf(stderr, "out of memory\n");
            bst_free(root);
            return EXIT_FAILURE;
        }
    printf("inserted:    50 30 70 20 40 60 80 35 45 65\n");
    print_rec("preorder:", bst_preorder, root);
    print_rec("inorder:", bst_inorder, root);
    print_rec("postorder:", bst_postorder, root);
    print_iter("level order:", bst_level_order, root);
    printf("size %zu, height %d\n", bst_size(root), bst_height(root));
    printf("contains 45: %s, contains 55: %s\n",
           bst_contains(root, 45) ? "yes" : "no", bst_contains(root, 55) ? "yes" : "no");
    printf("insert 40 again: %d\n", bst_insert(&root, 40));

    bst_remove(&root, 30);                  /* two children: 35 replaces it */
    printf("\nremove 30 (two children)\n");
    print_iter("inorder:", bst_inorder_iter, root);
    print_iter("level order:", bst_level_order, root);

    bst_remove(&root, 50);                  /* the root: 60 replaces it */
    bst_remove(&root, 20);                  /* a leaf */
    printf("\nremove 50 (root), 20 (leaf)\n");
    print_iter("inorder:", bst_inorder_iter, root);
    print_iter("level order:", bst_level_order, root);
    printf("size %zu, height %d\n", bst_size(root), bst_height(root));
    printf("remove 99: %s\n", bst_remove(&root, 99) ? "removed" : "not found");

    bst_free(root);
    return 0;
}
#endif
