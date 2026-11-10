#include <stdio.h>

int main(void)
{
    int scores[5] = {78, 92, 85, 64, 90};

    printf("sizeof scores = %zu bytes, sizeof scores[0] = %zu bytes\n",
           sizeof scores, sizeof scores[0]);
    printf("index  value  byte offset from scores[0]\n");
    for (int i = 0; i < 5; i++)
        printf("%5d  %5d  %5td\n", i, scores[i],
               (char *)&scores[i] - (char *)&scores[0]);
    return 0;
}
