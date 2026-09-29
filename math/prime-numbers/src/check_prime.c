/* check_prime.c - read one number and say whether it is prime.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic check_prime.c -o check_prime
 */
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static bool is_prime(int64_t n)
{
    if (n < 2)
        return false;
    if (n < 4)
        return true;
    if (n % 2 == 0 || n % 3 == 0)
        return false;
    for (int64_t i = 5; i <= n / i; i += 6)
        if (n % i == 0 || n % (i + 2) == 0)
            return false;
    return true;
}

int main(void)
{
    char line[64];
    printf("Enter an integer: ");
    fflush(stdout);                              /* show the prompt before reading */
    if (fgets(line, sizeof line, stdin) == NULL) {
        fputs("no input\n", stderr);
        return EXIT_FAILURE;
    }

    char *end;
    errno = 0;
    long long value = strtoll(line, &end, 10);   /* long long: at least 64 bits */
    bool no_digits = (end == line);              /* check before skipping spaces */
    while (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')
        end++;
    if (no_digits || *end != '\0') {
        fprintf(stderr, "not an integer\n");
        return EXIT_FAILURE;
    }
    if (errno == ERANGE) {
        fprintf(stderr, "out of range for a 64-bit integer\n");
        return EXIT_FAILURE;
    }

    int64_t n = (int64_t)value;
    printf("%" PRId64 " is %s\n", n, is_prime(n) ? "prime" : "not prime");
    return EXIT_SUCCESS;
}
