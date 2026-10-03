/* stability.c - selection sort's long-distance swap reorders equal keys */
#include <stdio.h>
#include <stddef.h>

typedef struct { int key; char tag; } Item;

int main(void)
{
    Item a[] = { {2,'a'}, {2,'b'}, {1,'c'} };
    size_t n = sizeof a / sizeof a[0];

    for (size_t i = 0; i + 1 < n; i++) {
        size_t min = i;
        for (size_t j = i + 1; j < n; j++)
            if (a[j].key < a[min].key)
                min = j;
        if (min != i) { Item t = a[i]; a[i] = a[min]; a[min] = t; }
    }
    for (size_t i = 0; i < n; i++)
        printf("%d%c ", a[i].key, a[i].tag);
    printf("\n");
    return 0;
}
