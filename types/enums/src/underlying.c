/* underlying: which integer type the compiler picked for each enum.
   The C standard leaves this to the implementation (C17 6.7.2.2), so
   the output differs between compilers. Enumeration constants
   themselves are always int before C23. */
#include <stdio.h>

enum color { RED, GREEN, BLUE };            /* no negative values */
enum temperature { COLD = -10, HOT = 40 };  /* a negative value */

#define TYPE_NAME(x) _Generic((x), \
    int: "int", unsigned int: "unsigned int", \
    long: "long", unsigned long: "unsigned long", \
    long long: "long long", unsigned long long: "unsigned long long", \
    char: "char", signed char: "signed char", unsigned char: "unsigned char", \
    short: "short", unsigned short: "unsigned short", \
    default: "another type")

int main(void)
{
    enum color c = GREEN;
    enum temperature t = HOT;
    (void)t;    /* _Generic only looks at the type; it never evaluates t */
    printf("the constant GREEN has type   %s\n", TYPE_NAME(GREEN));
    printf("enum color is compatible with %s\n", TYPE_NAME(c));
    printf("enum temperature is compatible with %s\n", TYPE_NAME(t));
    printf("sizeof(enum color) = %zu\n", sizeof c);

    /* A second test that works the same way on every compiler: store -1
       and read it back. A signed type keeps -1; an unsigned one wraps. */
    enum color probe = (enum color)-1;
    long long back = probe;
    printf("(enum color)-1 reads back as %lld: %s\n", back, back < 0 ? "signed" : "unsigned");
    return 0;
}
