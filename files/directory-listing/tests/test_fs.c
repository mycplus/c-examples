/* Tests for the directory layer and the tree walker.

   usage: test_fs FIXTURE_DIRECTORY SCRATCH_DIRECTORY
   FIXTURE_DIRECTORY is the tree make_fixture builds; SCRATCH_DIRECTORY is
   an empty place for the test's own files. */
#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L          /* symlink() for the link test */
#include <unistd.h>
#endif
#include "fs.h"
#include "walk.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks, failures;
#define CHECK(cond)                                                       \
    do {                                                                  \
        checks++;                                                         \
        if (!(cond)) {                                                    \
            failures++;                                                   \
            fprintf(stderr, "FAILED line %d: %s\n", __LINE__, #cond);     \
        }                                                                 \
    } while (0)

static void test_join(void)
{
    char buf[16];
    CHECK(fs_join(buf, sizeof buf, "dir", "file.txt") && strcmp(buf, "dir/file.txt") == 0);
    CHECK(fs_join(buf, sizeof buf, "dir/", "a") && strcmp(buf, "dir/a") == 0);       /* no double slash */
    CHECK(fs_join(buf, sizeof buf, "c:\\", "a") && strcmp(buf, "c:\\a") == 0);
    CHECK(fs_join(buf, sizeof buf, "", "a") && strcmp(buf, "a") == 0);
    CHECK(fs_join(buf, sizeof buf, "0123456", "0123456") && strlen(buf) == 15);      /* exactly fits */
    strcpy(buf, "unchanged");
    CHECK(!fs_join(buf, sizeof buf, "0123456", "01234567"));                         /* one byte too many */
    CHECK(strcmp(buf, "unchanged") == 0);
    strcpy(buf, "ab");
    CHECK(fs_join(buf, sizeof buf, buf, "cd") && strcmp(buf, "ab/cd") == 0);          /* dir is the buffer */
}

static void test_commas(void)
{
    char b[32];
    struct { long long v; const char *s; } cases[] = {
        {0, "0"}, {7, "7"}, {999, "999"}, {1000, "1,000"}, {6713, "6,713"},
        {1234567, "1,234,567"}, {-1234567, "-1,234,567"}, {100000, "100,000"},
        {LLONG_MAX, "9,223,372,036,854,775,807"},
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        with_commas(cases[i].v, b, sizeof b);
        CHECK(strcmp(b, cases[i].s) == 0);
    }
    char small_buf[4];
    with_commas(1234567, small_buf, sizeof small_buf);
    CHECK(strlen(small_buf) == 3);                                                   /* cut short, still terminated */
}

static void test_time(void)
{
    char b[32];
    fs_format_time(1700000000LL, true, b, sizeof b);
    CHECK(strcmp(b, "2023-11-14 22:13") == 0);
    fs_format_time(0, true, b, sizeof b);
    CHECK(strcmp(b, "1970-01-01 00:00") == 0);
}

static void test_list(const char *fixture)
{
    fs_entry *list;
    size_t n;
    char err[128];
    CHECK(fs_list(fixture, &list, &n, err, sizeof err));
    if (failures)
        return;
    static const char *const names[] = {"docs", "src", ".secret", "caf\xc3\xa9.txt", "empty.txt", "notes.txt"};
    static const long long sizes[] = {0, 0, 42, 7, 0, 10};
    CHECK(n == 6);
    for (size_t i = 0; i < n && i < 6; i++) {
        CHECK(strcmp(list[i].name, names[i]) == 0);
        CHECK(list[i].size == sizes[i]);
        CHECK((list[i].kind == FS_DIR) == (i < 2));
        CHECK(list[i].hidden == (i == 2));
    }
    CHECK(n >= 6 && list[5].mtime == 1717243200LL && list[4].mtime == 1700000000LL && list[0].mtime == 1700000000LL);
    free(list);

    CHECK(!fs_list("no-such-directory", &list, &n, err, sizeof err));
    CHECK(strcmp(err, "no such directory") == 0);
    char file[FS_PATH_MAX];
    CHECK(fs_join(file, sizeof file, fixture, "notes.txt"));
    CHECK(!fs_list(file, &list, &n, err, sizeof err));
    CHECK(strcmp(err, "not a directory") == 0 || strcmp(err, "no such directory") == 0);
}

static void test_empty_dir(const char *scratch)
{
    char dir[FS_PATH_MAX];
    CHECK(fs_join(dir, sizeof dir, scratch, "empty-dir") && fs_make(dir, -1, 1700000000LL));
    fs_entry *list;
    size_t n = 99;
    char err[128];
    CHECK(fs_list(dir, &list, &n, err, sizeof err) && n == 0);       /* "." and ".." are not entries */
    if (n == 0)
        free(list);
}

static void test_walk(const char *fixture)
{
    node root;
    CHECK(walk(fixture, -1, true, &root));
    CHECK(root.size == 6713 && root.count == 6);
    CHECK(root.count == 6 && root.children[0].size == 6234 && root.children[0].count == 2);
    CHECK(root.count == 6 && root.children[0].count == 2 && root.children[0].children[0].size == 5000);
    walk_free(&root);

    CHECK(walk(fixture, -1, false, &root));
    CHECK(root.size == 6671 && root.count == 5);                     /* without .secret */
    walk_free(&root);

    CHECK(walk(fixture, 0, true, &root));                            /* top level only */
    CHECK(root.count == 6 && root.children[0].count == 0 && root.size == 59);
    walk_free(&root);

    CHECK(walk("no-such-directory", -1, true, &root));
    CHECK(root.error != NULL && strcmp(root.error, "no such directory") == 0);
    walk_free(&root);
}

static void test_links(const char *scratch)
{
#if !defined(_WIN32)
    char dir[FS_PATH_MAX], link[FS_PATH_MAX];
    CHECK(fs_join(dir, sizeof dir, scratch, "with-link") && fs_make(dir, -1, 1700000000LL));
    CHECK(fs_join(link, sizeof link, dir, "self"));
    unlink(link);
    CHECK(symlink(".", link) == 0);
    fs_entry *list;
    size_t n;
    char err[128];
    CHECK(fs_list(dir, &list, &n, err, sizeof err) && n == 1);
    if (n == 1) {
        CHECK(list[0].kind == FS_LINK);
        free(list);
    }
    node root;
    CHECK(walk(dir, -1, true, &root));                               /* finishes: the loop is not followed */
    CHECK(root.count == 1 && root.children[0].kind == FS_LINK && root.children[0].count == 0);
    walk_free(&root);
#else
    (void)scratch;
#endif
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: test_fs FIXTURE_DIRECTORY SCRATCH_DIRECTORY\n");
        return 2;
    }
    if (!fs_make(argv[2], -1, 1700000000LL)) {
        fprintf(stderr, "test_fs: cannot create %s\n", argv[2]);
        return 2;
    }
    test_join();
    test_commas();
    test_time();
    test_list(argv[1]);
    test_empty_dir(argv[2]);
    test_walk(argv[1]);
    test_links(argv[2]);
    if (failures) {
        printf("%d of %d checks failed\n", failures, checks);
        return 1;
    }
    printf("all %d checks passed\n", checks);
    return 0;
}
