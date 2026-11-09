#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void swap_int(int *a, int *b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

/* One pass over n readings (n > 0), three results returned through pointers. */
static void stats(const double *v, size_t n, double *min, double *max, double *mean)
{
    double sum = v[0];
    *min = v[0];
    *max = v[0];
    for (size_t i = 1; i < n; i++) {
        if (v[i] < *min) *min = v[i];
        if (v[i] > *max) *max = v[i];
        sum += v[i];
    }
    *mean = sum / (double)n;
}

/* Returns true and stores the value only if the whole string is a valid int. */
static bool parse_int(const char *text, int *out)
{
    char *end;
    errno = 0;
    long v = strtol(text, &end, 10);
    if (end == text || *end != '\0' || errno == ERANGE || v < INT_MIN || v > INT_MAX)
        return false;
    *out = (int)v;
    return true;
}

int main(void)
{
    int x = 3, y = 9;
    swap_int(&x, &y);
    printf("after swap: x = %d, y = %d\n", x, y);

    double temps[] = {21.5, 19.0, 24.5, 22.0};
    double lo, hi, avg;
    stats(temps, sizeof temps / sizeof temps[0], &lo, &hi, &avg);
    printf("min %.1f, max %.1f, mean %.2f\n", lo, hi, avg);

    const char *inputs[] = {"42", "-7", "12abc", "", "99999999999"};
    for (size_t i = 0; i < sizeof inputs / sizeof inputs[0]; i++) {
        int n = 0;
        if (parse_int(inputs[i], &n))
            printf("\"%s\" -> %d\n", inputs[i], n);
        else
            printf("\"%s\" -> rejected\n", inputs[i]);
    }
    return 0;
}
