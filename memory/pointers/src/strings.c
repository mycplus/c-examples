#include <ctype.h>
#include <stdio.h>

/* Walks the string with a pointer; no index variable needed. */
static size_t count_words(const char *s)
{
    size_t words = 0;
    int in_word = 0;
    for (; *s != '\0'; s++) {
        if (isspace((unsigned char)*s)) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            words++;
        }
    }
    return words;
}

int main(void)
{
    const char *greeting = "hello";   /* points into read-only memory */
    char buffer[] = "hello";          /* a writable copy: 6 bytes with the '\0' */

    buffer[0] = 'J';
    printf("greeting: %s, sizeof(greeting) = %zu\n", greeting, sizeof greeting);
    printf("buffer:   %s, sizeof(buffer)   = %zu\n", buffer, sizeof buffer);

    const char *line = "  pointers   are just   addresses ";
    printf("\"%s\" has %zu words\n", line, count_words(line));
    return 0;
}
