/* Pitfall: forgetting to free the buffer from gdImagePngPtr(). Web code
   often encodes an image into memory to send it in a response instead of
   writing a file. The buffer belongs to the caller and must be released
   with gdFree(); gdImageDestroy() frees only the image.

   usage: png_buffer_leak [leak]      Without "leak" the buffer is freed. */
#include <gd.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    int leak = argc > 1 && strcmp(argv[1], "leak") == 0;
    gdImagePtr im = gdImageCreateTrueColor(100, 100);
    if (im == NULL)
        return 1;
    gdImageFilledRectangle(im, 0, 0, 99, 99, gdTrueColor(0x46, 0x7F, 0x40));

    int size = 0;
    void *png = gdImagePngPtr(im, &size);      /* the PNG file, in memory */
    gdImageDestroy(im);
    if (png == NULL)
        return 1;
    /* ... here a server would send the size bytes at png to the client ... */
    printf("encoded a PNG in memory%s\n", leak ? " (buffer not freed)" : " and freed it");
    if (!leak)
        gdFree(png);
    return 0;
}
