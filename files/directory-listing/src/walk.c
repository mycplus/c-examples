#include "walk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p != NULL)
        memcpy(p, s, n);
    return p;
}

/* path holds the directory's full path; it is extended in place for each
   child and restored afterwards, so the whole walk uses one buffer. */
static bool read_dir(char *path, size_t len, int depth_left, bool with_hidden, node *dir)
{
    fs_entry *list;
    size_t n;
    char err[128];
    if (!fs_list(path, &list, &n, err, sizeof err)) {
        if (strcmp(err, "out of memory") == 0)
            return false;
        dir->error = strstr(err, "no such") ? "no such directory"
                   : strstr(err, "not a directory") ? "not a directory"
                   : strstr(err, "permission") ? "permission denied" : "cannot be read";
        return true;
    }
    dir->children = calloc(n ? n : 1, sizeof *dir->children);
    if (dir->children == NULL) {
        free(list);
        return false;
    }
    bool ok = true;
    for (size_t i = 0; i < n && ok; i++) {
        if (list[i].hidden && !with_hidden)
            continue;
        node *c = &dir->children[dir->count];
        c->name = copy(list[i].name);
        if (c->name == NULL) {
            ok = false;
            break;
        }
        dir->count++;
        c->kind = list[i].kind;
        c->hidden = list[i].hidden;
        c->size = list[i].size;
        c->mtime = list[i].mtime;
        if (c->kind == FS_DIR && depth_left != 0) {
            if (!fs_join(path, FS_PATH_MAX, path, c->name)) {
                c->error = "path too long";
                continue;
            }
            ok = read_dir(path, strlen(path), depth_left - 1, with_hidden, c);
            path[len] = '\0';            /* back to this directory's path */
        }
        dir->size += c->size;
    }
    free(list);
    return ok;
}

bool walk(const char *path, int max_depth, bool with_hidden, node *root)
{
    memset(root, 0, sizeof *root);
    char buf[FS_PATH_MAX];
    if (strlen(path) >= sizeof buf || (root->name = copy(path)) == NULL)
        return false;
    strcpy(buf, path);
    root->kind = FS_DIR;
    return read_dir(buf, strlen(buf), max_depth, with_hidden, root);
}

void walk_free(node *n)
{
    for (size_t i = 0; i < n->count; i++)
        walk_free(&n->children[i]);
    free(n->children);
    free(n->name);
}

void with_commas(long long v, char *buf, size_t cap)
{
    char digits[32];
    int len = snprintf(digits, sizeof digits, "%lld", v < 0 ? -v : v);
    size_t out = 0;
    if (v < 0 && out + 1 < cap)
        buf[out++] = '-';
    for (int i = 0; i < len && out + 1 < cap; i++) {
        if (i > 0 && (len - i) % 3 == 0 && out + 1 < cap)
            buf[out++] = ',';
        if (out + 1 < cap)
            buf[out++] = digits[i];
    }
    buf[out] = '\0';
}
