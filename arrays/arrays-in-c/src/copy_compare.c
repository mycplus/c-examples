#include <stdio.h>
#include <string.h>

int main(void)
{
    int today[4] = {3, 1, 4, 1};
    int saved[4];

    memcpy(saved, today, sizeof today);          /* arrays cannot be assigned with = */
    today[0] = 9;

    printf("today: %d %d %d %d\n", today[0], today[1], today[2], today[3]);
    printf("saved: %d %d %d %d\n", saved[0], saved[1], saved[2], saved[3]);

    /* Compare contents with memcmp (or a loop), never with ==. */
    int again[4] = {3, 1, 4, 1};
    printf("saved and again hold the same values: %s\n",
           memcmp(saved, again, sizeof saved) == 0 ? "yes" : "no");
    return 0;
}
