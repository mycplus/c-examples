#include <stdio.h>

int main(void)
{
    int scores[5] = {78, 92, 85, 64, 90};        /* five ints, set at declaration */
    int empty[5] = {0};                          /* every element is 0 */
    int partial[5] = {1, 2};                     /* the rest are 0 */
    double prices[] = {2.50, 1.25, 4.00};        /* size taken from the list: 3 */
    int days_in_month[13] = {[1] = 31, [2] = 28, [3] = 31, [12] = 31};  /* C99 */

    printf("scores:  ");
    for (int i = 0; i < 5; i++)
        printf("%d ", scores[i]);
    printf("\nempty:   ");
    for (int i = 0; i < 5; i++)
        printf("%d ", empty[i]);
    printf("\npartial: ");
    for (int i = 0; i < 5; i++)
        printf("%d ", partial[i]);

    printf("\nprices has %zu elements; the first is %.2f\n",
           sizeof prices / sizeof prices[0], prices[0]);
    printf("February has %d days, April is not set: %d\n",
           days_in_month[2], days_in_month[4]);

    scores[3] = 70;                              /* change one element */
    printf("after the retake, scores[3] = %d\n", scores[3]);
    return 0;
}
