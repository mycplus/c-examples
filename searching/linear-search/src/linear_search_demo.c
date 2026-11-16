/* linear_search_demo.c - the examples from the article. */
#include <stdio.h>
#include <string.h>

#include "linear_search.h"

typedef struct {
    const char *name;
    double price;
} Product;

static bool has_name(const void *element, const void *ctx)
{
    return strcmp(((const Product *)element)->name, (const char *)ctx) == 0;
}

static void print_array(const char *label, const int *a, size_t n)
{
    printf("%-22s", label);
    for (size_t i = 0; i < n; i++)
        printf(" %d", a[i]);
    printf("\n");
}

int main(void)
{
    int a[] = {42, 7, 19, 73, 7, 0, -5, 88};
    const size_t n = sizeof a / sizeof a[0];
    const int keys[] = {73, 7, 0, 88, 50};

    print_array("array:", a, n);
    for (size_t k = 0; k < sizeof keys / sizeof keys[0]; k++) {
        ptrdiff_t at = linear_search(a, n, keys[k]);
        if (at >= 0)
            printf("%3d found at index %td after %td comparisons\n", keys[k], at, at + 1);
        else
            printf("%3d not found after %zu comparisons\n", keys[k], n);
    }

    ptrdiff_t found = sentinel_search(a, n, 88);
    ptrdiff_t missing = sentinel_search(a, n, 50);
    printf("sentinel search for 88: index %td, for 50: %td\n", found, missing);
    print_array("array afterwards:", a, n);

    const Product shelf[] = {{"pen", 1.25}, {"notebook", 3.5}, {"stapler", 7.0}};
    const Product *p = search_first(shelf, 3, sizeof shelf[0], has_name, "notebook");
    printf("search_first for \"notebook\": %s at index %td, price %.2f\n",
           p ? p->name : "none", p ? p - shelf : -1, p ? p->price : 0.0);

    int list[] = {1, 2, 3, 4, 5, 6};
    const int lookups[] = {5, 5, 6, 5};
    for (size_t k = 0; k < sizeof lookups / sizeof lookups[0]; k++) {
        char label[32];
        ptrdiff_t at = search_move_to_front(list, 6, lookups[k]);
        snprintf(label, sizeof label, "find %d (was at %td):", lookups[k], at);
        print_array(label, list, 6);
    }

    const int sorted[] = {-5, 0, 7, 7, 19, 42, 73, 88};
    printf("binary search in sorted copy for 7: index %td, for 50: %td\n",
           binary_search(sorted, 8, 7), binary_search(sorted, 8, 50));
    return 0;
}
