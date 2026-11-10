/* Real-life example: summarize one student's test scores. */
#include <stdio.h>

#define ARRAY_LEN(a) (sizeof (a) / sizeof (a)[0])

int main(void)
{
    const int scores[] = {78, 92, 85, 64, 90, 73};
    const size_t n = ARRAY_LEN(scores);

    int sum = 0, best = scores[0], worst = scores[0];
    for (size_t i = 0; i < n; i++) {
        sum += scores[i];
        if (scores[i] > best)
            best = scores[i];
        if (scores[i] < worst)
            worst = scores[i];
    }
    double average = (double)sum / (double)n;

    size_t above = 0;
    for (size_t i = 0; i < n; i++)
        if (scores[i] > average)
            above++;

    printf("%zu tests, total %d, average %.2f\n", n, sum, average);
    printf("best %d, worst %d, %zu above average\n", best, worst, above);
    return 0;
}
