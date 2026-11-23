/* imgcheck: prints what a file really is, for the tests:
   its format (from the first bytes), size, and the colors of given pixels.

   usage: imgcheck FILE [X Y]... */
#include "imgtools.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc < 2 || argc % 2 != 0) {
        fprintf(stderr, "usage: imgcheck FILE [X Y]...\n");
        return 2;
    }
    img_format fmt;
    int w, h;
    if (image_header(argv[1], &fmt, &w, &h) != 0) {
        printf("%s: not a PNG or JPEG file\n", argv[1]);
        return 1;
    }
    gdImagePtr im = gdImageCreateFromFile(argv[1]);
    if (im == NULL || gdImageSX(im) != w || gdImageSY(im) != h) {
        printf("%s: header says %dx%d but the image does not decode\n", argv[1], w, h);
        if (im != NULL)
            gdImageDestroy(im);
        return 1;
    }
    printf("%s: %s %dx%d\n", argv[1], fmt == IMG_PNG ? "PNG" : "JPEG", w, h);
    for (int i = 2; i + 1 < argc; i += 2) {
        int x = atoi(argv[i]), y = atoi(argv[i + 1]);
        int c = gdImageGetTrueColorPixel(im, x, y);
        printf("  (%d, %d) #%02X%02X%02X\n", x, y, gdTrueColorGetRed(c), gdTrueColorGetGreen(c),
               gdTrueColorGetBlue(c));
    }
    gdImageDestroy(im);
    return 0;
}
