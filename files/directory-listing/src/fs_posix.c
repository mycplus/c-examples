/* Directory listing on Linux, macOS and other POSIX systems. */
#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L          /* fstatat, dirfd, futimens */
#include "fs.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>

struct fs_dir {
    DIR *dir;
};

fs_dir *fs_opendir(const char *path, char *err, size_t err_len)
{
    fs_dir *d = malloc(sizeof *d);
    if (d == NULL) {
        snprintf(err, err_len, "out of memory");
        return NULL;
    }
    d->dir = opendir(path);
    if (d->dir == NULL) {
        snprintf(err, err_len, "%s", errno == ENOENT ? "no such directory"
                                   : errno == ENOTDIR ? "not a directory"
                                   : errno == EACCES ? "permission denied" : strerror(errno));
        free(d);
        return NULL;
    }
    return d;
}

int fs_readdir(fs_dir *d, fs_entry *e)
{
    for (;;) {
        errno = 0;
        struct dirent *de = readdir(d->dir);
        if (de == NULL)
            return errno == 0 ? 0 : -1;
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
        if (strlen(de->d_name) >= sizeof e->name)
            continue;                    /* cannot happen with NAME_MAX of 255 */

        /* fstatat looks the name up inside the directory being read, not the
           current directory, and AT_SYMLINK_NOFOLLOW reports a link as a link. */
        struct stat st;
        if (fstatat(dirfd(d->dir), de->d_name, &st, AT_SYMLINK_NOFOLLOW) != 0)
            continue;                    /* deleted since readdir saw it */
        strcpy(e->name, de->d_name);
        e->kind = S_ISDIR(st.st_mode) ? FS_DIR : S_ISREG(st.st_mode) ? FS_FILE
                : S_ISLNK(st.st_mode) ? FS_LINK : FS_OTHER;
        e->size = e->kind == FS_DIR ? 0 : (long long)st.st_size;
        e->mtime = (long long)st.st_mtime;
        e->hidden = de->d_name[0] == '.';
        return 1;
    }
}

void fs_console_utf8(void)
{
}

void fs_closedir(fs_dir *d)
{
    closedir(d->dir);
    free(d);
}

bool fs_free_space(const char *path, unsigned long long *bytes)
{
    struct statvfs v;
    if (statvfs(path, &v) != 0)
        return false;
    *bytes = (unsigned long long)v.f_bavail * v.f_frsize;
    return true;
}

void fs_format_time(long long t, bool utc, char *buf, size_t cap)
{
    time_t tt = (time_t)t;
    struct tm tm;
    if ((utc ? gmtime_r(&tt, &tm) : localtime_r(&tt, &tm)) == NULL || strftime(buf, cap, "%Y-%m-%d %H:%M", &tm) == 0)
        snprintf(buf, cap, "?");
}

bool fs_make(const char *path, long long size, long long mtime)
{
    int fd;
    if (size < 0) {
        if (mkdir(path, 0755) != 0 && errno != EEXIST)
            return false;
        fd = open(path, O_RDONLY);
    } else {
        fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0 && size > 0 && ftruncate(fd, (off_t)size) != 0) {
            close(fd);
            return false;
        }
    }
    if (fd < 0)
        return false;
    struct timespec times[2] = {{(time_t)mtime, 0}, {(time_t)mtime, 0}};
    bool ok = futimens(fd, times) == 0;
    return close(fd) == 0 && ok;
}
#else
typedef int fs_posix_unused;             /* ISO C forbids an empty file */
#endif
