#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Allocates an upper-case copy of text. The caller owns *out and must free it. */
static bool upper_copy(const char *text, char **out)
{
    size_t n = strlen(text);
    char *copy = malloc(n + 1);
    if (copy == NULL)
        return false;
    for (size_t i = 0; i <= n; i++)
        copy[i] = (char)toupper((unsigned char)text[i]);
    *out = copy;                 /* changes the caller's pointer, not a copy of it */
    return true;
}

int main(int argc, char **argv)
{
    printf("received %d argument(s)\n", argc - 1);
    for (char **arg = argv + 1; *arg != NULL; arg++) {
        char *shout = NULL;
        if (!upper_copy(*arg, &shout))
            return 1;
        printf("  %s -> %s\n", *arg, shout);
        free(shout);
    }
    return 0;
}
