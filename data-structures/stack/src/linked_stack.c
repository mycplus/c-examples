/* linked_stack.c - a stack of int built on a singly linked list.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic linked_stack.c -o linked_stack
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int          value;
    struct Node *next;   /* the node below this one */
} Node;

typedef struct {
    Node  *top;          /* NULL when the stack is empty */
    size_t size;
} LinkedStack;

void lstack_init(LinkedStack *s)
{
    s->top = NULL;
    s->size = 0;
}

bool lstack_is_empty(const LinkedStack *s)
{
    return s->top == NULL;
}

/* Returns false if the node could not be allocated; the stack is unchanged. */
bool lstack_push(LinkedStack *s, int value)
{
    Node *n = malloc(sizeof *n);
    if (n == NULL)
        return false;
    n->value = value;
    n->next = s->top;
    s->top = n;
    s->size++;
    return true;
}

/* Returns false on an empty stack and leaves *out untouched. */
bool lstack_pop(LinkedStack *s, int *out)
{
    Node *n = s->top;
    if (n == NULL)
        return false;
    *out = n->value;
    s->top = n->next;
    free(n);
    s->size--;
    return true;
}

bool lstack_peek(const LinkedStack *s, int *out)
{
    if (s->top == NULL)
        return false;
    *out = s->top->value;
    return true;
}

/* Iterative, so a million-node stack does not recurse a million deep. */
void lstack_free(LinkedStack *s)
{
    Node *n = s->top;
    while (n != NULL) {
        Node *next = n->next;
        free(n);
        n = next;
    }
    lstack_init(s);
}

int main(void)
{
    LinkedStack s;
    lstack_init(&s);
    int value = 0;

    printf("push 10 20 30\n");
    if (!lstack_push(&s, 10) || !lstack_push(&s, 20) || !lstack_push(&s, 30)) {
        fputs("out of memory\n", stderr);
        lstack_free(&s);
        return EXIT_FAILURE;
    }
    lstack_peek(&s, &value);
    printf("size=%zu top=%d\n", s.size, value);
    while (lstack_pop(&s, &value))
        printf("pop %d\n", value);
    printf("empty=%s\n", lstack_is_empty(&s) ? "true" : "false");
    printf("pop on empty: %s\n",
           lstack_pop(&s, &value) ? "returned a value" : "underflow reported");
    lstack_free(&s);
    return EXIT_SUCCESS;
}
