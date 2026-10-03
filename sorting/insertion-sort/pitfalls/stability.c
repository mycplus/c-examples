/* stability.c - "<=" instead of "<" still sorts, but reorders equal keys */
#include <stdio.h>
#include <stddef.h>

typedef struct { int key; char tag; } Item;

static void sort_items(Item a[], size_t n, int use_lte)
{
    for (size_t i = 1; i < n; i++) {
        Item v = a[i];
        size_t j = i;
        while (j > 0 && (use_lte ? v.key <= a[j - 1].key : v.key < a[j - 1].key)) {
            a[j] = a[j - 1];
            j--;
        }
        a[j] = v;
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
    show("input:         ", x, 5);
    sort_items(x, 5, 0);
    show("v <  a[j - 1]: ", x, 5);
    sort_items(y, 5, 1);
    show("v <= a[j - 1]: ", y, 5);
    return 0;
}
