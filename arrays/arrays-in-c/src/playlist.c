/* Real-life example: reverse a playlist in place. */
#include <stdio.h>

static void reverse(const char **items, size_t n)
{
    if (n < 2)
        return;
    for (size_t left = 0, right = n - 1; left < right; left++, right--) {
        const char *tmp = items[left];
        items[left] = items[right];
        items[right] = tmp;
    }
}

int main(void)
{
    const char *playlist[] = {"Intro", "Daylight", "Harbor", "Night Drive", "Outro"};
    const size_t n = sizeof playlist / sizeof playlist[0];

    reverse(playlist, n);
    for (size_t i = 0; i < n; i++)
        printf("%zu. %s\n", i + 1, playlist[i]);
    return 0;
}
