/* ties_right.c - taking from the right run on ties still sorts, but it
 * reverses the order of equal keys */
#include <stdio.h>
#include <string.h>
#include <stddef.h>

typedef struct { int key; char tag; } Item;

static void sort_items(Item a[], Item buf[], size_t lo, size_t hi, int ties_right)
{
    if (hi - lo < 2)
        return;
    size_t mid = lo + (hi - lo) / 2;
    sort_items(a, buf, lo, mid, ties_right);
    sort_items(a, buf, mid, hi, ties_right);
    size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        int take_right = ties_right ? a[j].key <= a[i].key    /* wrong */
                                    : a[j].key <  a[i].key;   /* right */
        buf[k++] = take_right ? a[j++] : a[i++];
    }
    while (i < mid) buf[k++] = a[i++];
    while (j < hi)  buf[k++] = a[j++];
    memcpy(a + lo, buf + lo, (hi - lo) * sizeof a[0]);
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
    Item x[] = { {2,'a'}, {1,'b'}, {2,'c'}, {1,'d'}, {2,'e'} }, bx[5];
    Item y[] = { {2,'a'}, {1,'b'}, {2,'c'}, {1,'d'}, {2,'e'} }, by[5];
    show("input:           ", x, 5);
    sort_items(x, bx, 0, 5, 0);
    show("right only if <: ", x, 5);
    sort_items(y, by, 0, 5, 1);
    show("right if <=:     ", y, 5);
    return 0;
}
