/* identicon: draws a default avatar from a user name, the way sites give
   new accounts a picture. The same name always gives the same image.

   usage: identicon NAME OUT.png */
#include "imgtools.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: identicon NAME OUT.png\n");
        return 2;
    }
    int cells[5][5], r, g, b;
    identicon_cells(argv[1], cells, &r, &g, &b);
    printf("%s: hash %08lx, color #%02X%02X%02X\n", argv[1],
           (unsigned long)fnv1a32(argv[1]), r, g, b);
    for (int row = 0; row < 5; row++) {        /* the same grid, as text */
        for (int col = 0; col < 5; col++)
            putchar(cells[row][col] ? '#' : '.');
        putchar('\n');
    }

    gdImagePtr im = draw_identicon(argv[1], 50);
    if (im == NULL || save_image(im, argv[2], -1) != 0) {
        fprintf(stderr, "identicon: cannot write %s\n", argv[2]);
        if (im != NULL)
            gdImageDestroy(im);
        return 1;
    }
    gdImageDestroy(im);
    return 0;
}
