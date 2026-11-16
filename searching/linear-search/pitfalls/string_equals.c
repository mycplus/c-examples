/* string_equals.c - searching an array of strings with == instead of
 * strcmp. == compares addresses, so it finds a key that happens to be the
 * same string literal and misses the same text typed in by a user. */
#include <stdio.h>
#include <string.h>

static const char *const fruits[] = {"apple", "banana", "cherry"};

static int search_eq(const char *key)
{
    for (int i = 0; i < 3; i++)
        if (fruits[i] == key)          /* compares pointers */
            return i;
    return -1;
}

static int search_strcmp(const char *key)
{
    for (int i = 0; i < 3; i++)
        if (strcmp(fruits[i], key) == 0)
            return i;
    return -1;
}

int main(void)
{
    char typed[16];
    strcpy(typed, "banana");           /* as if read with fgets */
    printf("==     with the literal \"banana\": %d\n", search_eq("banana"));
    printf("==     with typed text \"banana\":  %d\n", search_eq(typed));
    printf("strcmp with typed text \"banana\":  %d\n", search_strcmp(typed));
    return 0;
}
