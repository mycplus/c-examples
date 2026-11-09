#include <stdio.h>

int main(void)
{
    int score = 70;
    int *p = &score;          /* p holds the address of score */

    printf("score = %d\n", score);
    printf("*p    = %d\n", *p);
    printf("p == &score: %s\n", p == &score ? "yes" : "no");

    *p = *p + 21;             /* writes to score through the pointer */
    printf("after *p = *p + 21, score = %d\n", score);

    int *nothing = NULL;      /* points at no object at all */
    printf("nothing is %s\n", nothing == NULL ? "NULL" : "not NULL");
    return 0;
}
