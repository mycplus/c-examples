/* make_fixture: builds the small directory tree that the tests and the
   article's examples list. Every size and time is fixed, so the output is
   the same on every machine.

   usage: make_fixture DIRECTORY */
#include "fs.h"

#include <stdio.h>

#define NOV_14_2023 1700000000LL     /* 2023-11-14 22:13:20 UTC */
#define JUN_01_2024 1717243200LL     /* 2024-06-01 12:00:00 UTC */

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: make_fixture DIRECTORY\n");
        return 2;
    }
    static const struct { const char *path; long long size, mtime; } items[] = {
        {"", -1, 0},
        {"docs", -1, 0},
        {"docs/images", -1, 0},
        {"src", -1, 0},
        {"docs/guide.md", 1234, NOV_14_2023},
        {"docs/images/logo.png", 5000, NOV_14_2023},
        {"src/main.c", 300, NOV_14_2023},
        {"src/util.c", 120, NOV_14_2023},
        {".secret", 42, NOV_14_2023},
        {"empty.txt", 0, NOV_14_2023},
        {"notes.txt", 10, JUN_01_2024},
        {"caf\xc3\xa9.txt", 7, JUN_01_2024},     /* "café.txt" in UTF-8 */
        /* Directory times last: adding a file changes its directory's time. */
        {"docs/images", -1, NOV_14_2023},
        {"docs", -1, NOV_14_2023},
        {"src", -1, NOV_14_2023},
        {"", -1, NOV_14_2023},
    };
    char path[FS_PATH_MAX];
    for (size_t i = 0; i < sizeof items / sizeof items[0]; i++) {
        if (!fs_join(path, sizeof path, argv[1], items[i].path) ||
            !fs_make(items[i].path[0] ? path : argv[1], items[i].size, items[i].mtime)) {
            fprintf(stderr, "make_fixture: cannot create %s\n", path);
            return 1;
        }
    }
    return 0;
}
