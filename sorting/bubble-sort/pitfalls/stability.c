#include <stdio.h>
#include <stddef.h>

typedef struct { int key; char tag; } Item;

void sort_by_key(Item a[], size_t n, int use_lte)
{
    size_t bound = n;
    while (bound > 1) {
        size_t last_swap = 0;
        for (size_t j = 1; j < bound; j++) {
            int out_of_order = use_lte ? a[j].key <= a[j - 1].key
                                       : a[j].key <  a[j - 1].key;
            if (out_of_order) {
                Item t = a[j]; a[j] = a[j - 1]; a[j - 1] = t;
                last_swap = j;
            }
        }
        bound = last_swap;
    }
}

static void show(const char *label, const Item a[], size_t n)
{
    printf("%s", label);
    for (size_t i = 0; i < n; i++)
        printf(" %d%c", a[i].key, a[i].tag);
    printf("\n");
}

int main(void)
{
    Item x[] = { {2,'a'}, {1,'b'}, {2,'c'}, {1,'d'}, {2,'e'} };
    Item y[] = { {2,'a'}, {1,'b'}, {2,'c'}, {1,'d'}, {2,'e'} };
    size_t n = sizeof x / sizeof x[0];

    show("input:        ", x, n);
    sort_by_key(x, n, 0);
    show("strict  <  :  ", x, n);
    sort_by_key(y, n, 1);
    show("with    <= :  ", y, n);
    return 0;
}
