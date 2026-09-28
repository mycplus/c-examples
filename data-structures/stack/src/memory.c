/* memory.c - heap bytes used by one million ints in an array stack and in a
 * linked stack. Uses glibc's mallinfo2(), so it builds on Linux only; the
 * figures belong to glibc's allocator, not to C.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic memory.c -o memory
 */
#define _GNU_SOURCE
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int          value;
    struct Node *next;
} Node;

enum { COUNT = 1000000 };

static size_t heap_in_use(void)
{
    struct mallinfo2 m = mallinfo2();
    return m.uordblks + m.hblkhd;   /* arena bytes in use + mmapped bytes */
}

int main(void)
{
    size_t before = heap_in_use();
    int *items = NULL;
    size_t size = 0, capacity = 0;
    for (int i = 0; i < COUNT; ++i) {
        if (size == capacity) {
            capacity = capacity ? capacity * 2 : 8;
            int *p = realloc(items, capacity * sizeof *p);
            if (p == NULL) {
                free(items);
                return EXIT_FAILURE;
            }
            items = p;
        }
        items[size++] = i;
    }
    size_t array_bytes = heap_in_use() - before;

    before = heap_in_use();
    Node *top = NULL;
    for (int i = 0; i < COUNT; ++i) {
        Node *n = malloc(sizeof *n);
        if (n == NULL)
            break;                         /* freed below */
        n->value = i;
        n->next = top;
        top = n;
    }
    size_t linked_bytes = heap_in_use() - before;

    printf("sizeof(Node) = %zu bytes\n", sizeof(Node));
    printf("array stack:  %zu bytes for %d ints (capacity %zu)\n",
           array_bytes, COUNT, capacity);
    printf("linked stack: %zu bytes for %d ints\n", linked_bytes, COUNT);
    printf("ratio: %.1fx\n", (double)linked_bytes / (double)array_bytes);

    while (top != NULL) {
        Node *next = top->next;
        free(top);
        top = next;
    }
    free(items);
    return EXIT_SUCCESS;
}
