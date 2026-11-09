#include <stdio.h>
#include <stddef.h>

struct reading {
    char sensor;
    double value;
};

int main(void)
{
    char c[2];
    short s[2];
    int i[2];
    double d[2];
    struct reading r[2];

    printf("%-16s %-10s %s\n", "type", "sizeof", "p + 1 moves");
    printf("%-16s %-10zu %td bytes\n", "char", sizeof c[0], (char *)(c + 1) - (char *)c);
    printf("%-16s %-10zu %td bytes\n", "short", sizeof s[0], (char *)(s + 1) - (char *)s);
    printf("%-16s %-10zu %td bytes\n", "int", sizeof i[0], (char *)(i + 1) - (char *)i);
    printf("%-16s %-10zu %td bytes\n", "double", sizeof d[0], (char *)(d + 1) - (char *)d);
    printf("%-16s %-10zu %td bytes\n", "struct reading", sizeof r[0], (char *)(r + 1) - (char *)r);

    printf("\nsizeof(char *)           = %zu\n", sizeof(char *));
    printf("sizeof(double *)         = %zu\n", sizeof(double *));
    printf("sizeof(struct reading *) = %zu\n", sizeof(struct reading *));
    return 0;
}
