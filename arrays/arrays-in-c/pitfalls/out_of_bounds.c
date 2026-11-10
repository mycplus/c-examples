/* Deliberately broken: the loop reads one element past the end. */
#include <stdio.h>

int main(void)
{
    int scores[5] = {78, 92, 85, 64, 90};
    int total = 0;
    for (int i = 0; i <= 5; i++)                 /* should be i < 5 */
        total += scores[i];
    printf("total = %d\n", total);
    return 0;
}
