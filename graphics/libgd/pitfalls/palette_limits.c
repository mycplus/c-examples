/* Pitfall: using a palette image where a true-color image is needed.
   gdImageCreate() makes a palette image: at most 256 colors, and the first
   color allocated becomes the background. gdImageCreateTrueColor() stores
   a full RGB value per pixel. Resampling a photo or a gradient into a
   palette image throws most of its colors away. */
#include <gd.h>
#include <stdio.h>

/* Counts the different colors in an image (true color or palette). */
static long count_colors(gdImagePtr im)
{
    static unsigned char seen[1 << 24 >> 3];
    long n = 0;
    for (size_t i = 0; i < sizeof seen; i++)
        seen[i] = 0;
    for (int y = 0; y < gdImageSY(im); y++)
        for (int x = 0; x < gdImageSX(im); x++) {
            int c = gdImageGetTrueColorPixel(im, x, y) & 0xFFFFFF;
            if (!(seen[c >> 3] & (1 << (c & 7)))) {
                seen[c >> 3] |= (unsigned char)(1 << (c & 7));
                n++;
            }
        }
    return n;
}

int main(void)
{
    /* 1. A palette image holds 256 colors; the 257th allocation fails. */
    gdImagePtr pal = gdImageCreate(16, 16);
    if (pal == NULL)
        return 1;
    int ok = 0, failed = 0;
    for (int i = 0; i < 300; i++) {
        int c = gdImageColorAllocate(pal, i % 256, i / 256 * 100, 50);
        if (c == -1)
            failed++;
        else
            ok++;
    }
    printf("palette image: %d colors allocated, %d allocations returned -1\n", ok, failed);
    gdImageDestroy(pal);

    /* 2. Shrink a smooth 512 x 512 gradient into each kind of image. */
    gdImagePtr src = gdImageCreateTrueColor(512, 512);
    if (src == NULL)
        return 1;
    for (int y = 0; y < 512; y++)
        for (int x = 0; x < 512; x++)
            gdImageSetPixel(src, x, y, gdTrueColor(x / 2, y / 2, 128));
    printf("source gradient: %ld colors\n", count_colors(src));

    gdImagePtr to_tc = gdImageCreateTrueColor(128, 128);
    gdImagePtr to_pal = gdImageCreate(128, 128);
    if (to_tc == NULL || to_pal == NULL)
        return 1;
    gdImageCopyResampled(to_tc, src, 0, 0, 0, 0, 128, 128, 512, 512);
    gdImageCopyResampled(to_pal, src, 0, 0, 0, 0, 128, 128, 512, 512);
    printf("resampled into a true-color image: %ld colors\n", count_colors(to_tc));
    printf("resampled into a palette image:    %ld colors\n", count_colors(to_pal));
    gdImageDestroy(src);
    gdImageDestroy(to_tc);
    gdImageDestroy(to_pal);
    return 0;
}
