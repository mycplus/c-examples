/* dirlist: lists one directory, like "ls -l" on Linux or "dir" on Windows.

   usage: dirlist [-a] [-d | -f] [-u] [-s] [DIRECTORY]
     -a  include hidden entries      -d  directories only     -f  files only
     -u  show times in UTC           -s  show free space on the volume
   Exit status: 0 on success, 2 if the directory cannot be read. */
#include "fs.h"
#include "walk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    fs_console_utf8();
    bool all = false, dirs_only = false, files_only = false, utc = false, show_free = false;
    const char *path = ".";
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0)
            all = true;
        else if (strcmp(argv[i], "-d") == 0)
            dirs_only = true;
        else if (strcmp(argv[i], "-f") == 0)
            files_only = true;
        else if (strcmp(argv[i], "-u") == 0)
            utc = true;
        else if (strcmp(argv[i], "-s") == 0)
            show_free = true;
        else if (argv[i][0] == '-' || i != argc - 1) {
            fprintf(stderr, "usage: dirlist [-a] [-d | -f] [-u] [-s] [DIRECTORY]\n");
            return 2;
        } else
            path = argv[i];
    }

    fs_entry *list;
    size_t n;
    char err[128];
    if (!fs_list(path, &list, &n, err, sizeof err)) {
        fprintf(stderr, "dirlist: %s: %s\n", path, err);
        return 2;
    }

    printf("Directory: %s\n\n%-16s  %-5s  %13s  %s\n", path, utc ? "Modified (UTC)" : "Modified", "Type", "Size", "Name");
    size_t files = 0, dirs = 0, skipped = 0;
    long long bytes = 0;
    for (size_t i = 0; i < n; i++) {
        const fs_entry *e = &list[i];
        bool is_dir = e->kind == FS_DIR;
        if ((e->hidden && !all) || (dirs_only && !is_dir) || (files_only && is_dir)) {
            skipped++;
            continue;
        }
        char when[32], size[32];
        fs_format_time(e->mtime, utc, when, sizeof when);
        if (is_dir)
            size[0] = '\0';
        else
            with_commas(e->size, size, sizeof size);
        const char *type = is_dir ? "<DIR>" : e->kind == FS_LINK ? "<LNK>" : e->kind == FS_OTHER ? "<OTH>" : "";
        printf("%-16s  %-5s  %13s  %s\n", when, type, size, e->name);
        if (is_dir)
            dirs++;
        else {
            files++;
            bytes += e->size;
        }
    }
    char total[32];
    with_commas(bytes, total, sizeof total);
    printf("\n%zu director%s, %zu file%s, %s bytes", dirs, dirs == 1 ? "y" : "ies", files, files == 1 ? "" : "s", total);
    if (skipped)
        printf(" (%zu not shown)", skipped);
    putchar('\n');
    if (show_free) {
        unsigned long long free_bytes;
        if (fs_free_space(path, &free_bytes)) {
            with_commas((long long)free_bytes, total, sizeof total);
            printf("%s bytes free\n", total);
        }
    }
    free(list);
    return 0;
}
