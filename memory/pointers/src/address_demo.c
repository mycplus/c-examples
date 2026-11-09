#include <stdio.h>

int main(void)
{
    int score = 70;
    int *p = &score;

    printf("score: value %d, address %p\n", score, (void *)&score);
    printf("p:     value %p, address %p\n", (void *)p, (void *)&p);
    printf("sizeof(score) = %zu, sizeof(p) = %zu\n", sizeof score, sizeof p);
    return 0;
}
