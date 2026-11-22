/* Forgets the byte for the terminator, so strcpy writes one byte past the
 * end of the allocation. Often "works" in a normal build; AddressSanitizer
 * stops it with heap-buffer-overflow. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    const char *name = argc > 1 ? argv[1] : "defensive";
    char *copy = malloc(strlen(name)); /* WRONG: needs strlen(name) + 1 */
    if (copy == NULL) {
        return 1;
    }
    strcpy(copy, name);
    printf("%s\n", copy);
    free(copy);
    return 0;
}
