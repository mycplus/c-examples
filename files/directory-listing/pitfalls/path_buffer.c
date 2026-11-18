/* Pitfall: building a path in a fixed-size buffer with strcpy and strcat.

   Directory names and file names have no small fixed length, so
   "directory" + "/" + "name" can be longer than any buffer chosen in
   advance. strcpy and strcat do not check; fs_join does.

   usage: pitfall_path_buffer checked     (refuses the long path)
          pitfall_path_buffer unchecked   (writes past the buffer: undefined behavior) */
#include "fs.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    const char *dir = "projects/2024/client-reports";
    const char *name = "quarterly-summary-final-v2.xlsx";
    char path[32];

    if (argc > 1 && strcmp(argv[1], "unchecked") == 0) {
        strcpy(path, dir);                   /* 28 bytes + '\0': fits */
        strcat(path, "/");
        strcat(path, name);                  /* 61 bytes into a 32-byte buffer */
        printf("built %s\n", path);
        return 0;
    }

    if (fs_join(path, sizeof path, dir, name))
        printf("built %s\n", path);
    else
        printf("\"%s/%s\" needs %zu bytes; the buffer has %zu, so it was refused\n",
               dir, name, strlen(dir) + 1 + strlen(name) + 1, sizeof path);
    return 0;
}
