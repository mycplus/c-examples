/* first_image: the smallest useful LibGD program. It draws on a 320 x 200
   true-color image and saves it as PNG and as JPEG.

   Build:  cc first_image.c -lgd -o first_image */
#include <gd.h>
#include <gdfontl.h>
#include <stdio.h>

static int save(gdImagePtr im, const char *path, int is_png)
{
    FILE *out = fopen(path, "wb");             /* binary mode, always */
    if (out == NULL) {
        perror(path);
        return -1;
    }
    if (is_png)
        gdImagePng(im, out);
    else
        gdImageJpeg(im, out, 85);              /* quality 0-100 */
    return fclose(out) == 0 ? 0 : -1;
}

int main(void)
{
    gdImagePtr im = gdImageCreateTrueColor(320, 200);
    if (im == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    /* In a true-color image a color is just a packed RGB value. */
    int sky = gdTrueColor(0xDC, 0xEB, 0xF7);
    int blue = gdTrueColor(0x1A, 0x6F, 0xD4);
    int green = gdTrueColor(0x46, 0x7F, 0x40);
    int dark = gdTrueColor(0x1F, 0x29, 0x37);

    gdImageFilledRectangle(im, 0, 0, 319, 199, sky);          /* background */
    gdImageFilledRectangle(im, 0, 150, 319, 199, green);      /* ground */
    gdImageFilledEllipse(im, 260, 50, 60, 60, gdTrueColor(0xF2, 0xC1, 0x4E));
    gdImageFilledRectangle(im, 40, 90, 140, 150, blue);       /* a house */
    gdPoint roof[3] = {{30, 90}, {90, 50}, {150, 90}};
    gdImageFilledPolygon(im, roof, 3, dark);
    gdImageSetThickness(im, 3);
    gdImageLine(im, 180, 150, 300, 110, dark);                /* a thick line */
    gdImageSetThickness(im, 1);
    gdImageString(im, gdFontGetLarge(), 10, 10, (unsigned char *)"Made with LibGD", dark);

    int rc = save(im, "first.png", 1) == 0 && save(im, "first.jpg", 0) == 0 ? 0 : 1;
    gdImageDestroy(im);                        /* free the pixels */
    if (rc == 0)
        printf("wrote first.png and first.jpg (%d x %d)\n", 320, 200);
    return rc;
}
