/* stem.c - stems one word per line from standard input (or a file) and
 * prints one stem per line.
 * Usage: stem [--paper] [file]
 *        stem [--paper] --trace WORD   shows the word after every step */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "porter.h"

#define MAX_WORD 1024

static void show_step(const char *step, const char *word, void *ctx)
{
    (void)ctx;
    printf("  step %-3s %s\n", step, word);
}

static int usage(void)
{
    fputs("usage: stem [--paper] [file]\n       stem [--paper] --trace WORD\n", stderr);
    return 2;
}

int main(int argc, char **argv)
{
    enum porter_mode mode = PORTER_STANDARD;
    int i = 1;
    if (i < argc && strcmp(argv[i], "--paper") == 0) {
        mode = PORTER_PAPER;
        ++i;
    }
    if (i < argc && strcmp(argv[i], "--trace") == 0) {
        if (argc != i + 2 || strlen(argv[i + 1]) >= MAX_WORD)
            return usage();
        char word[MAX_WORD];
        strcpy(word, argv[i + 1]);
        for (char *p = word; *p; ++p)
            *p = (char)tolower((unsigned char)*p);
        printf("%s\n", word);
        porter_stem_traced(word, mode, show_step, NULL);
        printf("stem: %s\n", word);
        return 0;
    }
    if (argc > i + 1)
        return usage();

    FILE *in = stdin;
    if (i < argc && (in = fopen(argv[i], "r")) == NULL) {
        perror(argv[i]);
        return 1;
    }
    char line[MAX_WORD + 2];
    unsigned long n = 0;
    while (fgets(line, sizeof line, in) != NULL) {
        ++n;
        size_t len = strcspn(line, "\r\n");
        if (line[len] == '\0' && !feof(in)) {
            fprintf(stderr, "stem: line %lu is longer than %d characters\n", n, MAX_WORD);
            return 1;
        }
        line[len] = '\0';
        for (char *p = line; *p; ++p)
            *p = (char)tolower((unsigned char)*p);
        porter_stem(line, mode);
        puts(line);
    }
    if (in != stdin)
        fclose(in);
    return 0;
}
