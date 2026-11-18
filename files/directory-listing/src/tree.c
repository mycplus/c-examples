/* tree: prints the structure of a directory and everything below it.

   usage: tree [-a] [-s] [-L LEVELS] [DIRECTORY]
     -a  include hidden entries
     -s  show sizes (a directory's size is the total of everything below it)
     -L  descend at most LEVELS levels
   Exit status: 0 on success, 2 if the directory cannot be read. */
#include "walk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct totals {
    long long dirs, files;
};

static void print_children(const node *dir, char *prefix, size_t plen, int levels, bool sizes, struct totals *t)
{
    for (size_t i = 0; i < dir->count; i++) {
        const node *c = &dir->children[i];
        bool last = i + 1 == dir->count;
        printf("%s%s", prefix, last ? "`-- " : "|-- ");
        if (sizes) {
            char s[32];
            with_commas(c->size, s, sizeof s);
            printf("[%9s]  ", s);
        }
        printf("%s%s", c->name, c->kind == FS_DIR ? "/" : "");
        if (c->kind == FS_LINK)
            printf("  (link, not followed)");
        if (c->error)
            printf("  [%s]", c->error);
        putchar('\n');

        if (c->kind == FS_DIR) {
            t->dirs++;
            if (levels != 1 && plen + 5 < FS_PATH_MAX) {
                strcpy(prefix + plen, last ? "    " : "|   ");
                print_children(c, prefix, plen + 4, levels - 1, sizes, t);
                prefix[plen] = '\0';
            }
        } else
            t->files++;
    }
}

int main(int argc, char **argv)
{
    fs_console_utf8();
    bool all = false, sizes = false;
    int levels = -1;
    const char *path = ".";
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0)
            all = true;
        else if (strcmp(argv[i], "-s") == 0)
            sizes = true;
        else if (strcmp(argv[i], "-L") == 0 && i + 1 < argc && atoi(argv[i + 1]) > 0)
            levels = atoi(argv[++i]);
        else if (argv[i][0] == '-' || i != argc - 1) {
            fprintf(stderr, "usage: tree [-a] [-s] [-L LEVELS] [DIRECTORY]\n");
            return 2;
        } else
            path = argv[i];
    }

    node root;
    /* -L 1 shows only the directory's own entries. With -s the whole tree is
       still read, so a directory's size includes the levels not printed. */
    if (!walk(path, levels < 0 || sizes ? -1 : levels - 1, all, &root)) {
        fprintf(stderr, "tree: out of memory\n");
        walk_free(&root);
        return 2;
    }
    if (root.error) {
        fprintf(stderr, "tree: %s: %s\n", path, root.error);
        walk_free(&root);
        return 2;
    }

    if (sizes) {
        char s[32];
        with_commas(root.size, s, sizeof s);
        printf("[%9s]  ", s);
    }
    printf("%s\n", path);
    char prefix[FS_PATH_MAX] = "";
    struct totals t = {0, 0};
    print_children(&root, prefix, 0, levels, sizes, &t);
    printf("\n%lld director%s, %lld file%s\n", t.dirs, t.dirs == 1 ? "y" : "ies", t.files, t.files == 1 ? "" : "s");
    walk_free(&root);
    return 0;
}
