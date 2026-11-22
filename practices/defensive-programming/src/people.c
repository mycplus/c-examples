/* people: reads "name,age" lines, rejects bad records with a reason and a
 * line number, and prints the valid ones. Usage: people FILE */
#include "defensive.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define NAME_MAX_LEN 32
#define LINE_MAX_LEN 80

typedef struct {
    char name[NAME_MAX_LEN + 1];
    long age;
} person;

/* Checked when compiling: parse_person relies on this. */
static_assert(sizeof(((person *)0)->name) == NAME_MAX_LEN + 1,
              "name must hold NAME_MAX_LEN characters and the terminator");

/* Splits "name,age" and validates both fields.
 * Returns NULL on success, otherwise the reason the record was rejected. */
static const char *parse_person(const char *line, person *out)
{
    if (line[0] == '\0') {
        return "empty line";
    }
    const char *comma = strchr(line, ',');
    if (comma == NULL) {
        return "missing comma";
    }
    const size_t name_len = (size_t)(comma - line);
    if (name_len == 0) {
        return "empty name";
    }
    if (name_len > NAME_MAX_LEN) {
        return "name longer than 32 characters";
    }
    long age;
    const dp_status st = dp_parse_long(comma + 1, 0, 150, &age);
    if (st != DP_OK) {
        return dp_strerror(st);
    }
    memcpy(out->name, line, name_len);
    out->name[name_len] = '\0';
    out->age = age;
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s FILE\n", argv[0]);
        return 2;
    }
    FILE *in = fopen(argv[1], "r");
    if (in == NULL) {
        fprintf(stderr, "%s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    char line[LINE_MAX_LEN + 1]; /* 80 characters and the terminator */
    unsigned long number = 0;
    int accepted = 0;
    int rejected = 0;
    dp_status st;
    while ((st = dp_read_line(in, line, sizeof line)) != DP_ERR_EOF) {
        number++;
        person p = {{0}, 0}; /* initialized, so no path can print garbage */
        const char *why = NULL;
        if (st == DP_ERR_TOO_LONG) {
            why = "line longer than 80 characters";
        } else if (st != DP_OK) {
            why = dp_strerror(st);
        } else if ((why = parse_person(line, &p)) == NULL) {
            printf("ok    line %lu: %s, %ld\n", number, p.name, p.age);
            accepted++;
            continue;
        }
        printf("error line %lu: %s\n", number, why);
        rejected++;
        if (st == DP_ERR_IO) {
            break;
        }
    }
    const int read_failed = ferror(in);
    fclose(in);
    printf("%d accepted, %d rejected\n", accepted, rejected);
    return (rejected == 0 && !read_failed) ? 0 : 1;
}
