/* The parts of the directory layer that are the same on every platform. */
#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool fs_join(char *buf, size_t cap, const char *dir, const char *name)
{
    size_t d = strlen(dir), n = strlen(name);
    bool slash = d > 0 && dir[d - 1] != '/' && dir[d - 1] != '\\';
    if (d + slash + n + 1 > cap)
        return false;
    memmove(buf, dir, d);            /* dir may already be buf */
    if (slash)
        buf[d++] = '/';              /* Windows accepts '/' as well as '\\' */
    memcpy(buf + d, name, n + 1);
    return true;
}

static int compare_entries(const void *a, const void *b)
{
    const fs_entry *x = a, *y = b;
    int xd = x->kind == FS_DIR, yd = y->kind == FS_DIR;
    if (xd != yd)
        return yd - xd;              /* directories first */
    return strcmp(x->name, y->name);
}

bool fs_list(const char *path, fs_entry **out, size_t *count, char *err, size_t err_len)
{
    fs_dir *d = fs_opendir(path, err, err_len);
    if (d == NULL)
        return false;
    size_t n = 0, cap = 16;
    fs_entry *list = malloc(cap * sizeof *list);
    int r = 1;
    while (list != NULL && (r = fs_readdir(d, &list[n])) == 1) {
        if (++n == cap) {
            fs_entry *bigger = realloc(list, 2 * cap * sizeof *list);
            if (bigger == NULL) {
                free(list);
                list = NULL;
                break;
            }
            list = bigger;
            cap *= 2;
        }
    }
    fs_closedir(d);
    if (list == NULL || r < 0) {
        snprintf(err, err_len, list == NULL ? "out of memory" : "error while reading the directory");
        free(list);
        return false;
    }
    qsort(list, n, sizeof *list, compare_entries);
    *out = list;
    *count = n;
    return true;
}
