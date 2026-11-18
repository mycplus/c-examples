/* Directory listing on Windows. It uses the wide-character (UTF-16) API,
   so names outside the current code page survive, and converts every name
   to UTF-8 for the rest of the program. */
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct fs_dir {
    HANDLE find;
    WIN32_FIND_DATAW data;
    bool have_pending;         /* FindFirstFileW already returned the first entry */
};

static bool to_wide(const char *utf8, wchar_t *out, int cap)
{
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, out, cap) > 0;
}

static bool to_utf8(const wchar_t *wide, char *out, int cap)
{
    return WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, cap, NULL, NULL) > 0;
}

/* Seconds since 1970 from a FILETIME (100 ns ticks since 1601). */
static long long from_filetime(FILETIME ft)
{
    ULARGE_INTEGER v;
    v.LowPart = ft.dwLowDateTime;
    v.HighPart = ft.dwHighDateTime;
    return (long long)((v.QuadPart - 116444736000000000ULL) / 10000000ULL);
}

static FILETIME to_filetime(long long t)
{
    ULARGE_INTEGER v;
    v.QuadPart = (ULONGLONG)t * 10000000ULL + 116444736000000000ULL;
    FILETIME ft;
    ft.dwLowDateTime = v.LowPart;
    ft.dwHighDateTime = v.HighPart;
    return ft;
}

fs_dir *fs_opendir(const char *path, char *err, size_t err_len)
{
    char pattern[FS_PATH_MAX];
    wchar_t wpattern[FS_PATH_MAX];
    if (!fs_join(pattern, sizeof pattern, path, "*") || !to_wide(pattern, wpattern, FS_PATH_MAX)) {
        snprintf(err, err_len, "path too long or not valid UTF-8");
        return NULL;
    }
    fs_dir *d = malloc(sizeof *d);
    if (d == NULL) {
        snprintf(err, err_len, "out of memory");
        return NULL;
    }
    d->find = FindFirstFileW(wpattern, &d->data);
    if (d->find == INVALID_HANDLE_VALUE) {
        DWORD code = GetLastError();
        if (code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND)
            snprintf(err, err_len, "no such directory");
        else if (code == ERROR_DIRECTORY)
            snprintf(err, err_len, "not a directory");
        else if (code == ERROR_ACCESS_DENIED)
            snprintf(err, err_len, "permission denied");
        else
            snprintf(err, err_len, "Windows error %lu", (unsigned long)code);
        free(d);
        return NULL;
    }
    d->have_pending = true;
    return d;
}

int fs_readdir(fs_dir *d, fs_entry *e)
{
    for (;;) {
        if (d->have_pending)
            d->have_pending = false;
        else if (!FindNextFileW(d->find, &d->data))
            return GetLastError() == ERROR_NO_MORE_FILES ? 0 : -1;

        const WIN32_FIND_DATAW *f = &d->data;
        if (wcscmp(f->cFileName, L".") == 0 || wcscmp(f->cFileName, L"..") == 0)
            continue;
        if (!to_utf8(f->cFileName, e->name, (int)sizeof e->name))
            continue;
        DWORD a = f->dwFileAttributes;
        if ((a & FILE_ATTRIBUTE_REPARSE_POINT) && f->dwReserved0 == IO_REPARSE_TAG_SYMLINK)
            e->kind = FS_LINK;
        else if (a & FILE_ATTRIBUTE_DIRECTORY)
            e->kind = FS_DIR;
        else
            e->kind = FS_FILE;
        e->size = e->kind == FS_DIR ? 0 : (long long)(((ULONGLONG)f->nFileSizeHigh << 32) | f->nFileSizeLow);
        e->mtime = from_filetime(f->ftLastWriteTime);
        e->hidden = e->name[0] == '.' || (a & FILE_ATTRIBUTE_HIDDEN) != 0;
        return 1;
    }
}

void fs_console_utf8(void)
{
    SetConsoleOutputCP(CP_UTF8);
}

void fs_closedir(fs_dir *d)
{
    FindClose(d->find);
    free(d);
}

bool fs_free_space(const char *path, unsigned long long *bytes)
{
    wchar_t wpath[FS_PATH_MAX];
    ULARGE_INTEGER avail;
    if (!to_wide(path, wpath, FS_PATH_MAX) || !GetDiskFreeSpaceExW(wpath, &avail, NULL, NULL))
        return false;
    *bytes = avail.QuadPart;
    return true;
}

void fs_format_time(long long t, bool utc, char *buf, size_t cap)
{
    time_t tt = (time_t)t;
    struct tm *tm = utc ? gmtime(&tt) : localtime(&tt);
    if (tm == NULL || strftime(buf, cap, "%Y-%m-%d %H:%M", tm) == 0)
        snprintf(buf, cap, "?");
}

bool fs_make(const char *path, long long size, long long mtime)
{
    wchar_t wpath[FS_PATH_MAX];
    if (!to_wide(path, wpath, FS_PATH_MAX))
        return false;
    if (size < 0 && !CreateDirectoryW(wpath, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        return false;
    HANDLE h = CreateFileW(wpath, GENERIC_WRITE, 0, NULL, size < 0 ? OPEN_EXISTING : CREATE_ALWAYS,
                           size < 0 ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    bool ok = true;
    if (size > 0) {
        LARGE_INTEGER end;
        end.QuadPart = size;
        ok = SetFilePointerEx(h, end, NULL, FILE_BEGIN) && SetEndOfFile(h);
    }
    FILETIME ft = to_filetime(mtime);
    ok = ok && SetFileTime(h, NULL, &ft, &ft);
    return CloseHandle(h) && ok;
}
#else
typedef int fs_win32_unused;             /* ISO C forbids an empty file */
#endif
