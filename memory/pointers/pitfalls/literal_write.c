/* Deliberately broken: modifies a string literal. */
#include <stdio.h>

int main(void)
{
    char *s = "hello";
    s[0] = 'J';               /* string literals live in read-only memory */
    printf("%s\n", s);
    return 0;
}
