/* Pitfall: using the result of a load function without checking it.
   gdImageCreateFromFile() and friends return NULL for a missing, truncated
   or corrupt file. gdImageSX() then reads through a null pointer.

   usage: unchecked_load FILE [unchecked] */
#include <gd.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc < 2)
        return 2;
    gdImagePtr im = gdImageCreateFromFile(argv[1]);
    if (argc > 2 && strcmp(argv[2], "unchecked") == 0) {
        printf("size: %d x %d\n", gdImageSX(im), gdImageSY(im));   /* crashes if im is NULL */
        return 0;
    }
    if (im == NULL) {
        printf("cannot load the file as an image\n");
        return 1;
    }
    printf("size: %d x %d\n", gdImageSX(im), gdImageSY(im));
    gdImageDestroy(im);
    return 0;
}
