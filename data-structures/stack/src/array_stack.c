/* array_stack.c - a growable array-backed stack of int, with a
 * bracket checker as a worked use case.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic array_stack.c -o array_stack
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int   *items;     /* heap block holding the elements        */
    size_t size;      /* elements in use; items[size-1] is top  */
    size_t capacity;  /* elements allocated                     */
} IntStack;

void stack_init(IntStack *s)
{
    s->items = NULL;
    s->size = 0;
    s->capacity = 0;
}

void stack_free(IntStack *s)
{
    free(s->items);
    stack_init(s);
}

bool stack_is_empty(const IntStack *s)
{
    return s->size == 0;
}

/* Returns false if the stack could not grow; the stack is unchanged. */
bool stack_push(IntStack *s, int value)
{
    if (s->size == s->capacity) {
        if (s->capacity > SIZE_MAX / 2 / sizeof *s->items)
            return false;                      /* byte count would overflow */
        size_t new_cap = s->capacity ? s->capacity * 2 : 8;
        int *p = realloc(s->items, new_cap * sizeof *p);
        if (p == NULL)
            return false;                      /* old block is still valid  */
        s->items = p;
        s->capacity = new_cap;
    }
    s->items[s->size++] = value;
    return true;
}

/* Returns false on an empty stack and leaves *out untouched. */
bool stack_pop(IntStack *s, int *out)
{
    if (s->size == 0)
        return false;
    *out = s->items[--s->size];
    return true;
}

bool stack_peek(const IntStack *s, int *out)
{
    if (s->size == 0)
        return false;
    *out = s->items[s->size - 1];
    return true;
}

static int opener_for(char close)
{
    return close == ')' ? '(' : close == ']' ? '[' : '{';
}

/* Allocation failure is reported as "not balanced". */
bool balanced(const char *text)
{
    IntStack s;
    stack_init(&s);
    bool ok = true;
    for (const char *p = text; *p != '\0' && ok; ++p) {
        if (*p == '(' || *p == '[' || *p == '{') {
            ok = stack_push(&s, *p);
        } else if (*p == ')' || *p == ']' || *p == '}') {
            int open = 0;
            ok = stack_pop(&s, &open) && open == opener_for(*p);
        }
    }
    ok = ok && stack_is_empty(&s);
    stack_free(&s);
    return ok;
}

int main(void)
{
    IntStack s;
    stack_init(&s);
    int value = 0;

    printf("push 10 20 30\n");
    if (!stack_push(&s, 10) || !stack_push(&s, 20) || !stack_push(&s, 30)) {
        fputs("out of memory\n", stderr);
        stack_free(&s);
        return EXIT_FAILURE;
    }
    stack_peek(&s, &value);
    printf("size=%zu top=%d\n", s.size, value);
    while (stack_pop(&s, &value))
        printf("pop %d\n", value);
    printf("empty=%s\n", stack_is_empty(&s) ? "true" : "false");
    printf("pop on empty: %s\n",
           stack_pop(&s, &value) ? "returned a value" : "underflow reported");
    stack_free(&s);

    const char *tests[] = { "{[()()]}", "([)]", "((", "())", "" };
    for (size_t i = 0; i < sizeof tests / sizeof tests[0]; ++i)
        printf("balanced(\"%s\") = %s\n", tests[i],
               balanced(tests[i]) ? "true" : "false");
    return EXIT_SUCCESS;
}
