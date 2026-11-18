/* The shortest correct way to list a directory on Windows. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

int main(void)
{
    WIN32_FIND_DATAW data;
    HANDLE find = FindFirstFileW(L".\\*", &data);    /* the pattern, not the directory */
    if (find == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "FindFirstFileW failed: error %lu\n", (unsigned long)GetLastError());
        return 1;
    }
    do {
        wprintf(L"%ls\n", data.cFileName);          /* includes "." and ".." */
    } while (FindNextFileW(find, &data));
    FindClose(find);
    return 0;
}
