/* free_literal.c - a function that returns malloc'd memory for most inputs
 * but a string literal for an empty result. The caller cannot tell the two
 * apart, so freeing every result frees memory malloc never gave out.
 * Run it under AddressSanitizer to see the report. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Returns the characters a and b share, in a's order (a stand-in for any
 * function that builds a string): "" when there are none. */
static char *common_letters(const char *a, const char *b)
{
    char *out = malloc(strlen(a) + 1);
    size_t k = 0;
    if (out == NULL)
        return NULL;
    for (const char *p = a; *p != '\0'; ++p)
        if (strchr(b, *p) != NULL)
            out[k++] = *p;
    if (k == 0) {
        free(out);
        return "";                     /* BUG: not from malloc */
    }
    out[k] = '\0';
    return out;
}

int main(void)
{
    static const char *const pairs[][2] = {{"hello", "world"}, {"abc", "xyz"}};
    for (size_t i = 0; i < 2; ++i) {
        char *r = common_letters(pairs[i][0], pairs[i][1]);
        printf("%s / %s -> \"%s\"\n", pairs[i][0], pairs[i][1], r);
        fflush(stdout);
        free(r);                       /* the second call frees a literal */
    }
    return 0;
}
