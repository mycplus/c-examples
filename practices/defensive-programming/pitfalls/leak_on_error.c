/* Returns early on an error and forgets to free the buffer. The program
 * runs fine; GCC's -fanalyzer reports the leak without running it. */
#include <stdio.h>
#include <stdlib.h>

int count_bytes(const char *path, long *out)
{
    char *buffer = malloc(4096);
    if (buffer == NULL) {
        return -1;
    }
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return -1; /* WRONG: buffer leaks on this path */
    }
    long total = 0;
    size_t n;
    while ((n = fread(buffer, 1, 4096, f)) > 0) {
        total += (long)n;
    }
    fclose(f);
    free(buffer);
    *out = total;
    return 0;
}

int main(int argc, char *argv[])
{
    long n = 0;
    if (argc > 1 && count_bytes(argv[1], &n) == 0) {
        printf("%ld bytes\n", n);
    }
    return 0;
}
