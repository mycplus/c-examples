/* thumbnail: makes a thumbnail of a PNG or JPEG that fits in a square box,
   keeping the aspect ratio, the way an upload handler does.

   usage: thumbnail IN OUT MAX_SIDE      (OUT ends in .png, .jpg or .jpeg)

   The size is read from the file header first, so a huge or malformed
   image is rejected before LibGD decodes it. */
#include "imgtools.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_PIXELS 40000000L                   /* 40 megapixels */

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: thumbnail IN OUT MAX_SIDE\n");
        return 2;
    }
    int max_side = atoi(argv[3]);
    if (max_side < 1 || max_side > 4096 || format_from_name(argv[2]) == IMG_UNKNOWN) {
        fprintf(stderr, "thumbnail: MAX_SIDE must be 1-4096 and OUT a .png or .jpg name\n");
        return 2;
    }

    img_format fmt;
    int w, h;
    if (image_header(argv[1], &fmt, &w, &h) != 0) {
        fprintf(stderr, "thumbnail: %s is not a PNG or JPEG file\n", argv[1]);
        return 1;
    }
    if ((long)w * h > MAX_PIXELS) {
        fprintf(stderr, "thumbnail: %s is %d x %d, larger than allowed\n", argv[1], w, h);
        return 1;
    }

    gdImagePtr src = gdImageCreateFromFile(argv[1]);
    if (src == NULL) {
        fprintf(stderr, "thumbnail: cannot decode %s\n", argv[1]);
        return 1;
    }
    gdImagePtr thumb = make_thumbnail(src, max_side);
    int rc = thumb != NULL && save_image(thumb, argv[2], 85) == 0 ? 0 : 1;
    if (rc == 0)
        printf("%s %dx%d -> %s %dx%d\n", argv[1], gdImageSX(src), gdImageSY(src),
               argv[2], gdImageSX(thumb), gdImageSY(thumb));
    else
        fprintf(stderr, "thumbnail: cannot write %s\n", argv[2]);
    if (thumb != NULL)
        gdImageDestroy(thumb);
    gdImageDestroy(src);
    return rc;
}
