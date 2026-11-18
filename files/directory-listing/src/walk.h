#ifndef WALK_H
#define WALK_H

#include "fs.h"

/* A directory tree read into memory, so it can be sorted, summed and
   printed in any order. Symbolic links are recorded but never followed,
   so a link that points back up the tree cannot cause endless recursion. */
typedef struct node {
    char *name;
    fs_kind kind;
    bool hidden;
    long long size;            /* for a directory: the total of everything below it */
    long long mtime;
    const char *error;         /* why a directory could not be read, or NULL */
    struct node *children;
    size_t count;
} node;

/* Reads path and everything below it, up to max_depth levels (-1 = no limit).
   Hidden entries are skipped unless with_hidden. Returns false only when
   memory runs out; unreadable subdirectories are marked with an error. */
bool walk(const char *path, int max_depth, bool with_hidden, node *root);

void walk_free(node *n);

/* "1,234,567": thousands separators without depending on the locale. */
void with_commas(long long v, char *buf, size_t cap);

#endif
