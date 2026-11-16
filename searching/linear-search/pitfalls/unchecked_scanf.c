/* unchecked_scanf.c - reading numbers until a 0 without checking what
 * scanf returns. Given "5 7 x 0", scanf stops at the x and leaves it in the
 * input, so the 0 is never read. The loop is capped at 10 rounds here;
 * without the cap it would never end. */
#include <stdio.h>

int main(void)
{
    int value = 0, rounds = 0, sum = 0;
    do {
        scanf("%d", &value);           /* return value ignored */
        sum += value;
        rounds++;
    } while (value != 0 && rounds < 10);
    printf("rounds: %d, last value: %d, sum: %d\n", rounds, value, sum);
    return 0;
}
