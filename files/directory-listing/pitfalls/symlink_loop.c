/* Pitfall: following symbolic links while walking a directory tree.

   A link that points back at its own directory ("self" -> ".") makes the
   tree infinitely deep to any walker that follows links. stat() follows
   them; lstat() and fstatat(..., AT_SYMLINK_NOFOLLOW) do not. Here the
   following walker only stops because Linux refuses to resolve a path
   that goes through more than 40 symbolic links. (POSIX only.)

   usage: pitfall_symlink_loop WORK_DIRECTORY   (creates WORK_DIRECTORY/loop) */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char path[8192];
static int visited;
static size_t loop_at;                   /* where "loop/..." starts in path */

/* Descends into "self" as long as stat() says it is a directory. */
static void walk_following(size_t len, int depth)
{
    struct stat st;
    snprintf(path + len, sizeof path - len, "/self");
    if (stat(path, &st) != 0) {
        printf("  stopped at depth %d: %s\n", depth, strerror(errno));
        return;
    }
    visited++;
    if (depth < 3 || depth > 38)
    {
        const char *shown = path + loop_at;
        size_t len_shown = strlen(shown);
        printf("  depth %2d: %s%s\n", depth, len_shown > 34 ? "..." : "", len_shown > 34 ? shown + len_shown - 31 : shown);
    }
    else if (depth == 3)
        printf("  ...\n");
    if (S_ISDIR(st.st_mode))
        walk_following(strlen(path), depth + 1);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: pitfall_symlink_loop WORK_DIRECTORY\n");
        return 2;
    }
    snprintf(path, sizeof path, "%s/loop", argv[1]);
    loop_at = strlen(path) - strlen("loop");
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        perror(path);
        return 2;
    }
    char link[sizeof path + 8];
    snprintf(link, sizeof link, "%s/self", path);
    unlink(link);
    if (symlink(".", link) != 0) {
        perror(link);
        return 2;
    }

    struct stat st;
    printf("lstat(\"loop/self\"): %s\n", lstat(link, &st) == 0 && S_ISLNK(st.st_mode) ? "a symbolic link, not followed" : "?");
    printf("stat(\"loop/self\"):  %s\n\n", stat(link, &st) == 0 && S_ISDIR(st.st_mode) ? "a directory (the link was followed)" : "?");

    printf("A walker that follows links:\n");
    walk_following(strlen(path), 1);
    printf("%d directories visited, all of them the same one\n", visited);
    return 0;
}
