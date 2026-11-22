/* Reads memory after free(). AddressSanitizer reports heap-use-after-free. */
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int *values = malloc(4 * sizeof *values);
    if (values == NULL) {
        return 1;
    }
    values[0] = 42;
    free(values);
    printf("%d\n", values[0]); /* WRONG: values points to freed memory */
    return 0;
}
