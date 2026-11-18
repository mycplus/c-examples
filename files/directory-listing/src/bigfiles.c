/* bigfiles: finds the largest files below a directory, the first question
   when a disk is filling up.

   usage: bigfiles [-a] DIRECTORY [COUNT]     (COUNT defaults to 10)
   Exit status: 0 on success, 2 if the directory cannot be read. */
#include "walk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct found {
    long long size;
    char path[FS_PATH_MAX];
};

struct list {
    struct found *items;
    size_t count, cap;
};

/* Collects every file below n, with its path relative to the starting directory. */
static bool collect(const node *n, char *rel, size_t len, struct list *out)
{
    for (size_t i = 0; i < n->count; i++) {
        const node *c = &n->children[i];
        char *path = rel;
        if (!fs_join(path, FS_PATH_MAX, len ? rel : "", c->name))
            continue;
        if (c->kind == FS_DIR) {
            if (!collect(c, path, strlen(path), out))
                return false;
        } else if (c->kind == FS_FILE) {
            if (out->count == out->cap) {
                size_t cap = out->cap ? out->cap * 2 : 64;
                struct found *bigger = realloc(out->items, cap * sizeof *bigger);
                if (bigger == NULL)
                    return false;
                out->items = bigger;
                out->cap = cap;
            }
            out->items[out->count].size = c->size;
            strcpy(out->items[out->count].path, path);
            out->count++;
        }
        rel[len] = '\0';
    }
    return true;
}

static int larger_first(const void *a, const void *b)
{
    const struct found *x = a, *y = b;
    if (x->size != y->size)
        return x->size < y->size ? 1 : -1;
    return strcmp(x->path, y->path);     /* ties in a fixed order */
}

int main(int argc, char **argv)
{
    fs_console_utf8();
    int first = 1;
    bool all = argc > 1 && strcmp(argv[1], "-a") == 0;
    if (all)
        first = 2;
    if (argc - first < 1 || argc - first > 2) {
        fprintf(stderr, "usage: bigfiles [-a] DIRECTORY [COUNT]\n");
        return 2;
    }
    const char *path = argv[first];
    long count = argc - first == 2 ? strtol(argv[first + 1], NULL, 10) : 10;
    if (count < 1)
        count = 10;

    node root;
    if (!walk(path, -1, all, &root) || root.error) {
        fprintf(stderr, "bigfiles: %s: %s\n", path, root.error ? root.error : "out of memory");
        walk_free(&root);
        return 2;
    }
    struct list found = {NULL, 0, 0};
    char rel[FS_PATH_MAX] = "";
    bool ok = collect(&root, rel, 0, &found);
    if (ok) {
        qsort(found.items, found.count, sizeof *found.items, larger_first);
        char total[32];
        with_commas(root.size, total, sizeof total);
        printf("%zu files, %s bytes in %s\n\n", found.count, total, path);
        for (size_t i = 0; i < found.count && i < (size_t)count; i++) {
            char s[32];
            with_commas(found.items[i].size, s, sizeof s);
            int pct = root.size ? (int)(found.items[i].size * 100 / root.size) : 0;
            printf("%13s  %3d%%  %s\n", s, pct, found.items[i].path);
        }
    } else
        fprintf(stderr, "bigfiles: out of memory\n");
    free(found.items);
    walk_free(&root);
    return ok ? 0 : 2;
}
