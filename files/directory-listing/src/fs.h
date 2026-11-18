#ifndef FS_H
#define FS_H

/* A small portable layer over directory listing: POSIX opendir/readdir/fstatat
   on Linux and macOS, FindFirstFileW/FindNextFileW on Windows. Names are UTF-8
   on every platform, and "." and ".." are never returned. */

#include <stdbool.h>
#include <stddef.h>

typedef enum { FS_FILE, FS_DIR, FS_LINK, FS_OTHER } fs_kind;

#define FS_NAME_MAX 1024     /* bytes of UTF-8, including the terminating '\0' */
#define FS_PATH_MAX 4096

typedef struct {
    char name[FS_NAME_MAX];
    fs_kind kind;            /* a symbolic link is reported as a link, never followed */
    long long size;          /* bytes; 0 for directories */
    long long mtime;         /* last modification, seconds since 1970-01-01 UTC */
    bool hidden;             /* name starts with '.', or (Windows) the hidden attribute */
} fs_entry;

typedef struct fs_dir fs_dir;

/* Opens a directory for reading. On failure returns NULL and writes a
   reason ("no such directory", "permission denied", ...) to err. */
fs_dir *fs_opendir(const char *path, char *err, size_t err_len);

/* Reads the next entry: returns 1 and fills *e, 0 at the end, -1 on error. */
int fs_readdir(fs_dir *d, fs_entry *e);

void fs_closedir(fs_dir *d);

/* Reads a whole directory into a new array sorted with directories first,
   then by name (byte order). Returns false on error; free(*out) when done. */
bool fs_list(const char *path, fs_entry **out, size_t *count, char *err, size_t err_len);

/* Builds "dir/name" into buf. Returns false, leaving buf unchanged, if the
   result would not fit: a path is never silently cut short. */
bool fs_join(char *buf, size_t cap, const char *dir, const char *name);

/* Free space on the volume holding path, for the current user. */
bool fs_free_space(const char *path, unsigned long long *bytes);

/* Formats a time as "YYYY-MM-DD HH:MM", in UTC or local time. */
void fs_format_time(long long t, bool utc, char *buf, size_t cap);

/* Creates a file of the given size and modification time, or a directory
   (size < 0), with a UTF-8 path. Used by the test fixture and the demos. */
bool fs_make(const char *path, long long size, long long mtime);

/* Windows: switches the console to UTF-8 so names such as "café.txt" display
   correctly. Does nothing on other systems, where terminals already use UTF-8. */
void fs_console_utf8(void);

#endif
