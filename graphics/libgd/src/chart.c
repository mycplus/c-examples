/* chart: draws a bar chart as a PNG file from "label value" lines, the
   kind of image a web dashboard or a nightly report generates on the fly.

   usage: chart DATA OUT.png [FONT.ttf]
   With a TrueType font (and LibGD built with FreeType) the title uses it. */
#include "imgtools.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "usage: chart DATA OUT.png [FONT.ttf]\n");
        return 2;
    }
    bar bars[24];
    int n = read_bars(argv[1], bars, 24);
    if (n <= 0) {
        fprintf(stderr, "chart: no data in %s\n", argv[1]);
        return 2;
    }
    int freetype;
    gdImagePtr im = draw_bar_chart(bars, n, 480, 300, "Downloads per month",
                                   argc == 4 ? argv[3] : NULL, &freetype);
    if (im == NULL) {
        fprintf(stderr, "chart: cannot draw the chart\n");
        return 1;
    }
    int rc = save_image(im, argv[2], -1);
    gdImageDestroy(im);
    if (rc != 0) {
        fprintf(stderr, "chart: cannot write %s\n", argv[2]);
        return 1;
    }
    printf("%s: %d bars, title in %s\n", argv[2], n,
           freetype ? "a TrueType font" : "LibGD's built-in font");
    return 0;
}
