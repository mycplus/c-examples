/* Real-life example: a week of temperature readings, processed by functions. */
#include <stdio.h>

/* The array arrives as a pointer, so the length must be passed too. */
static double average(const double *t, size_t n)
{
    double sum = 0.0;
    for (size_t i = 0; i < n; i++)
        sum += t[i];
    return n > 0 ? sum / (double)n : 0.0;
}

/* Changes the caller's array: no copy was made. */
static void to_fahrenheit(double *t, size_t n)
{
    for (size_t i = 0; i < n; i++)
        t[i] = t[i] * 9.0 / 5.0 + 32.0;
}

/* A function cannot return an array, so the caller supplies the space. */
static size_t days_above(const double *t, size_t n, double limit, int *out)
{
    size_t count = 0;
    for (size_t i = 0; i < n; i++)
        if (t[i] > limit)
            out[count++] = (int)i + 1;           /* day numbers start at 1 */
    return count;
}

int main(void)
{
    double week[7] = {21.0, 23.5, 25.0, 19.5, 22.0, 27.5, 26.0};
    const size_t n = sizeof week / sizeof week[0];

    printf("average: %.2f C\n", average(week, n));

    int hot[7];
    size_t k = days_above(week, n, 24.0, hot);
    printf("days above 24 C:");
    for (size_t i = 0; i < k; i++)
        printf(" %d", hot[i]);
    printf("\n");

    to_fahrenheit(week, n);
    printf("in Fahrenheit:");
    for (size_t i = 0; i < n; i++)
        printf(" %.1f", week[i]);
    printf("\n");
    return 0;
}
