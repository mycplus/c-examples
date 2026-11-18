/* Pitfall: calling stat() on the name that readdir() returns.

   readdir() gives a bare name such as "notes.txt", not a path. stat()
   resolves it against the current directory, so unless the program happens
   to be running inside the directory it is listing, every call fails or,
   worse, finds a different file with the same name. (POSIX only.)

   usage: pitfall_stat_wrong_path DIRECTORY */
#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: pitfall_stat_wrong_path DIRECTORY\n");
        return 2;
    }
    DIR *d = opendir(argv[1]);
    if (d == NULL) {
        perror(argv[1]);
        return 2;
    }
    /* Read the names first, so the two passes print in a fixed order. */
    char names[32][256];
    int n = 0;
    struct dirent *de;
    while ((de = readdir(d)) != NULL && n < 32)
        if (de->d_name[0] != '.')
            snprintf(names[n++], sizeof names[0], "%s", de->d_name);
    for (int i = 0; i < n; i++)              /* sort for stable output */
        for (int j = i + 1; j < n; j++)
            if (strcmp(names[i], names[j]) > 0) {
                char t[256];
                strcpy(t, names[i]);
                strcpy(names[i], names[j]);
                strcpy(names[j], t);
            }

    int bad = 0, good = 0;
    printf("%-24s %-20s %s\n", "stat(name)", "fstatat(dir, name)", "name");
    for (int i = 0; i < n; i++) {
        struct stat st;
        char wrong[64], right[64];
        if (stat(names[i], &st) == 0)
            snprintf(wrong, sizeof wrong, "%lld bytes", (long long)st.st_size);
        else {
            snprintf(wrong, sizeof wrong, "fails: %s", errno == ENOENT ? "no such file" : strerror(errno));
            bad++;
        }
        if (fstatat(dirfd(d), names[i], &st, AT_SYMLINK_NOFOLLOW) == 0) {
            snprintf(right, sizeof right, "%lld bytes", (long long)st.st_size);
            good++;
        } else
            snprintf(right, sizeof right, "fails");
        printf("%-24s %-20s %s\n", wrong, S_ISDIR(st.st_mode) ? "directory" : right, names[i]);
    }
    printf("stat(name) failed for %d of %d entries; fstatat() for %d\n", bad, n, n - good);
    closedir(d);
    return 0;
}
