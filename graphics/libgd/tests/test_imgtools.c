/* Unit tests for imgtools. Run from a writable directory: they create and
   delete a few small image files.  usage: test_imgtools DATA_FILE */
#include "imgtools.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks, failures;
#define CHECK(cond)                                                       \
    do {                                                                  \
        checks++;                                                         \
        if (!(cond)) {                                                    \
            failures++;                                                   \
            if (failures <= 20)                                           \
                fprintf(stderr, "FAILED line %d: %s\n", __LINE__, #cond); \
        }                                                                 \
    } while (0)

static int is_rgb(gdImagePtr im, int x, int y, int rgb)
{
    return (gdImageGetTrueColorPixel(im, x, y) & 0xFFFFFF) == rgb;
}

static void test_names(void)
{
    CHECK(format_from_name("a.png") == IMG_PNG);
    CHECK(format_from_name("A.PNG") == IMG_PNG);
    CHECK(format_from_name("photo.jpg") == IMG_JPEG);
    CHECK(format_from_name("photo.JPEG") == IMG_JPEG);
    CHECK(format_from_name("photo.gif") == IMG_UNKNOWN);
    CHECK(format_from_name("png") == IMG_UNKNOWN);
    CHECK(format_from_name("") == IMG_UNKNOWN);
}

static void test_fit_within(void)
{
    static const int cases[][5] = {
        /* w, h, max, expected w, expected h */
        {1600, 1200, 200, 200, 150},
        {1200, 1600, 200, 150, 200},
        {300, 199, 200, 200, 133},     /* 132.67 rounds to 133 */
        {199, 300, 200, 133, 200},
        {100, 50, 200, 100, 50},       /* never enlarged */
        {200, 200, 200, 200, 200},
        {201, 201, 200, 200, 200},
        {4000, 10, 100, 100, 1},       /* never below 1 pixel */
        {10, 4000, 100, 1, 100},
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        int w, h;
        fit_within(cases[i][0], cases[i][1], cases[i][2], &w, &h);
        CHECK(w == cases[i][3] && h == cases[i][4]);
    }
    /* Every result fits in the box and keeps the aspect ratio within half a pixel. */
    for (int w = 1; w <= 400; w += 7)
        for (int h = 1; h <= 400; h += 11) {
            int tw, th;
            fit_within(w, h, 100, &tw, &th);
            CHECK(tw >= 1 && th >= 1 && tw <= 100 && th <= 100 && tw <= w && th <= h);
            if (w > 100 || h > 100) {
                CHECK(tw == 100 || th == 100);
                double want = w >= h ? (double)h * 100 / w : (double)w * 100 / h;
                double got = w >= h ? th : tw;
                CHECK((got - want <= 0.5 + 1e-9 && want - got <= 0.5 + 1e-9) || (got == 1 && want < 1));
            }
        }
}

static void test_files(void)
{
    gdImagePtr im = gdImageCreateTrueColor(37, 23);
    CHECK(im != NULL);
    if (im == NULL)
        return;
    gdImageFilledRectangle(im, 0, 0, 36, 22, gdTrueColor(10, 20, 30));
    gdImageSetPixel(im, 5, 7, gdTrueColor(200, 100, 50));

    CHECK(save_image(im, "t-image.png", -1) == 0);
    CHECK(save_image(im, "t-image.jpg", 90) == 0);
    CHECK(save_image(im, "t-image.gif", -1) == -1);               /* unknown extension */
    CHECK(save_image(im, "no-such-dir/t.png", -1) == -1);         /* cannot open */
    CHECK(save_image(NULL, "t-null.png", -1) == -1);

    img_format f;
    int w = 0, h = 0;
    CHECK(image_header("t-image.png", &f, &w, &h) == 0 && f == IMG_PNG && w == 37 && h == 23);
    w = h = 0;
    CHECK(image_header("t-image.jpg", &f, &w, &h) == 0 && f == IMG_JPEG && w == 37 && h == 23);
    CHECK(image_header("no-such-file.png", &f, &w, &h) == -1);

    /* PNG is lossless: every pixel comes back exactly. */
    gdImagePtr back = gdImageCreateFromFile("t-image.png");
    CHECK(back != NULL);
    if (back != NULL) {
        CHECK(gdImageSX(back) == 37 && gdImageSY(back) == 23);
        CHECK(is_rgb(back, 5, 7, 200 << 16 | 100 << 8 | 50));
        CHECK(is_rgb(back, 0, 0, 10 << 16 | 20 << 8 | 30));
        gdImageDestroy(back);
    }

    /* A truncated PNG and a text file are both rejected. */
    FILE *in = fopen("t-image.png", "rb"), *out = fopen("t-cut.png", "wb");
    unsigned char buf[20];
    if (in != NULL && out != NULL) {
        size_t n = fread(buf, 1, sizeof buf, in);
        fwrite(buf, 1, n, out);
    }
    if (in != NULL)
        fclose(in);
    if (out != NULL)
        fclose(out);
    CHECK(image_header("t-cut.png", &f, &w, &h) == -1);
    out = fopen("t-text.png", "wb");
    if (out != NULL) {
        fputs("this is not an image\n", out);
        fclose(out);
    }
    CHECK(image_header("t-text.png", &f, &w, &h) == -1);
    CHECK(gdImageCreateFromFile("t-text.png") == NULL);

    /* A thumbnail of a 37 x 23 image in a 10-pixel box is 10 x 6. */
    gdImagePtr thumb = make_thumbnail(im, 10);
    CHECK(thumb != NULL);
    if (thumb != NULL) {
        CHECK(gdImageSX(thumb) == 10 && gdImageSY(thumb) == 6);
        CHECK(gdImageTrueColor(thumb));
        gdImageDestroy(thumb);
    }
    gdImageDestroy(im);
    remove("t-image.png");
    remove("t-image.jpg");
    remove("t-cut.png");
    remove("t-text.png");
}

static void test_identicon(void)
{
    CHECK(fnv1a32("") == 0x811c9dc5u);         /* published FNV-1a test vectors */
    CHECK(fnv1a32("a") == 0xe40c292cu);
    CHECK(fnv1a32("foobar") == 0xbf9cf968u);

    static const char *names[] = {"alice", "bob", "carol", "dave", "Alice", "mycplus", "x"};
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
        int cells[5][5], again[5][5], r, g, b, r2, g2, b2;
        identicon_cells(names[i], cells, &r, &g, &b);
        identicon_cells(names[i], again, &r2, &g2, &b2);
        CHECK(memcmp(cells, again, sizeof cells) == 0 && r == r2 && g == g2 && b == b2);
        CHECK(r >= 40 && r < 200 && g >= 40 && g < 200 && b >= 40 && b < 200);
        uint32_t h = fnv1a32(names[i]);
        for (int row = 0; row < 5; row++)
            for (int col = 0; col < 5; col++) {
                CHECK(cells[row][col] == cells[row][4 - col]);           /* mirrored */
                int src = col < 3 ? col : 4 - col;
                CHECK(cells[row][col] == (int)(h >> (row * 3 + src) & 1u));
            }

        gdImagePtr im = draw_identicon(names[i], 10);
        CHECK(im != NULL);
        if (im == NULL)
            continue;
        CHECK(gdImageSX(im) == 60 && gdImageSY(im) == 60);
        CHECK(is_rgb(im, 0, 0, 0xF0F0F0));
        for (int row = 0; row < 5; row++)
            for (int col = 0; col < 5; col++) {
                int x = 5 + col * 10 + 5, y = 5 + row * 10 + 5;
                CHECK(is_rgb(im, x, y, cells[row][col] ? (r << 16 | g << 8 | b) : 0xF0F0F0));
            }
        gdImageDestroy(im);
    }
    int a[5][5], b2[5][5], r, g, b;
    identicon_cells("alice", a, &r, &g, &b);
    identicon_cells("Alice", b2, &r, &g, &b);
    CHECK(memcmp(a, b2, sizeof a) != 0);       /* different names, different pictures */
}

static void test_chart(const char *data)
{
    bar bars[24];
    CHECK(read_bars("no-such-file.txt", bars, 24) == -1);
    int n = read_bars(data, bars, 24);
    CHECK(n == 6);
    if (n != 6)
        return;
    CHECK(strcmp(bars[0].label, "Jan") == 0 && bars[0].value == 1200);
    CHECK(strcmp(bars[5].label, "Jun") == 0 && bars[5].value == 2900);

    CHECK(bar_height_px(2900, 3150, 220) == 203);   /* 202.5 rounds up */
    CHECK(bar_height_px(3150, 3150, 220) == 220);
    CHECK(bar_height_px(0, 3150, 220) == 0);
    CHECK(bar_height_px(5, 0, 220) == 0);
    CHECK(draw_bar_chart(bars, 0, 480, 300, "t", NULL, NULL) == NULL);

    int ft = -1;
    gdImagePtr im = draw_bar_chart(bars, n, 480, 300, "Downloads", NULL, &ft);
    CHECK(im != NULL && ft == 0);
    if (im == NULL)
        return;
    CHECK(gdImageSX(im) == 480 && gdImageSY(im) == 300);
    int base = 300 - CHART_BOTTOM, plot_h = base - CHART_TOP;
    long max = 3150;
    int slot = (480 - CHART_RIGHT - CHART_LEFT) / n, bw = slot * 3 / 5;
    for (int i = 0; i < n; i++) {
        int bh = bar_height_px(bars[i].value, max, plot_h);
        int bx = CHART_LEFT + i * slot + (slot - bw) / 2, cx = bx + bw / 2;
        CHECK(is_rgb(im, cx, base - 1, CHART_BAR_COLOR));        /* bottom of the bar */
        CHECK(is_rgb(im, cx, base - bh, CHART_BAR_COLOR));       /* its top row */
        CHECK(!is_rgb(im, cx, base - bh - 1, CHART_BAR_COLOR));  /* nothing above */
        CHECK(is_rgb(im, bx, base - 1, CHART_BAR_COLOR) && is_rgb(im, bx + bw - 1, base - 1, CHART_BAR_COLOR));
        CHECK(!is_rgb(im, bx - 1, base - 1, CHART_BAR_COLOR) && !is_rgb(im, bx + bw, base - 1, CHART_BAR_COLOR));
    }
    /* Exactly n blue runs along the bottom row of the plot. */
    int runs = 0;
    for (int x = 1; x < 480; x++)
        if (is_rgb(im, x, base - 1, CHART_BAR_COLOR) && !is_rgb(im, x - 1, base - 1, CHART_BAR_COLOR))
            runs++;
    CHECK(runs == n);
    gdImageDestroy(im);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: test_imgtools DATA_FILE\n");
        return 2;
    }
    test_names();
    test_fit_within();
    test_files();
    test_identicon();
    test_chart(argv[1]);
    if (failures) {
        printf("%d of %d checks failed\n", failures, checks);
        return 1;
    }
    printf("all %d checks passed\n", checks);
    return 0;
}
