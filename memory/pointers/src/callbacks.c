#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct employee {
    const char *name;
    int salary;
};

static int by_name(const void *a, const void *b)
{
    const struct employee *x = a;     /* qsort passes const void *: cast it back */
    const struct employee *y = b;
    return strcmp(x->name, y->name);
}

/* qsort is not stable, so ties on salary fall back to the name. */
static int by_salary_desc(const void *a, const void *b)
{
    const struct employee *x = a;
    const struct employee *y = b;
    if (x->salary != y->salary)
        return (y->salary > x->salary) - (y->salary < x->salary);
    return by_name(a, b);
}

typedef double (*binary_op)(double, double);

static double add(double a, double b) { return a + b; }
static double sub(double a, double b) { return a - b; }
static double mul(double a, double b) { return a * b; }
static double divide(double a, double b) { return b != 0.0 ? a / b : 0.0; }

static const struct {
    char symbol;
    binary_op apply;
} operations[] = {
    {'+', add}, {'-', sub}, {'*', mul}, {'/', divide},
};

static void print_staff(const char *heading, const struct employee *e, size_t n)
{
    printf("%s\n", heading);
    for (size_t i = 0; i < n; i++)
        printf("  %-6s %d\n", e[i].name, e[i].salary);
}

int main(void)
{
    struct employee staff[] = {
        {"Maya", 52000}, {"Omar", 61000}, {"Lena", 48000}, {"Raj", 61000},
    };
    size_t n = sizeof staff / sizeof staff[0];

    qsort(staff, n, sizeof staff[0], by_name);
    print_staff("By name:", staff, n);

    qsort(staff, n, sizeof staff[0], by_salary_desc);
    print_staff("By salary, highest first:", staff, n);

    printf("Calculator table:\n");
    for (size_t i = 0; i < sizeof operations / sizeof operations[0]; i++)
        printf("  12 %c 4 = %g\n", operations[i].symbol, operations[i].apply(12.0, 4.0));
    return 0;
}
