/* The shortest correct way to list a directory on Linux or macOS. */
#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : ".";
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror(path);
        return 1;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
        printf("%s\n", entry->d_name);       /* includes "." and "..", in no particular order */
    closedir(dir);
    return 0;
}
