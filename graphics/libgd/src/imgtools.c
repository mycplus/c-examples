#include "imgtools.h"

#include <gdfontl.h>
#include <gdfonts.h>
#include <gdfontt.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* ---- Files ---- */

static int ends_with(const char *s, const char *suffix)
{
    size_t n = strlen(s), m = strlen(suffix);
    if (m > n)
        return 0;
    for (size_t i = 0; i < m; i++)
        if (tolower((unsigned char)s[n - m + i]) != suffix[i])
            return 0;
    return 1;
}

img_format format_from_name(const char *path)
{
    if (ends_with(path, ".png"))
        return IMG_PNG;
    if (ends_with(path, ".jpg") || ends_with(path, ".jpeg"))
        return IMG_JPEG;
    return IMG_UNKNOWN;
}

static unsigned be16(const unsigned char *p) { return (unsigned)p[0] << 8 | p[1]; }
static unsigned long be32(const unsigned char *p)
{
    return (unsigned long)p[0] << 24 | (unsigned long)p[1] << 16 | (unsigned long)p[2] << 8 | p[3];
}

/* JPEG: walk the markers until a start-of-frame marker, which holds the size. */
static int jpeg_size(FILE *f, int *w, int *h)
{
    unsigned char b[7];
    for (;;) {
        int c = fgetc(f);
        while (c == 0xFF)                     /* fill bytes before a marker */
            c = fgetc(f);
        if (c == EOF)
            return -1;
        if (c == 0xD8 || c == 0x01 || (c >= 0xD0 && c <= 0xD7))
            continue;                         /* markers without a length */
        if (fread(b, 1, 2, f) != 2)
            return -1;
        unsigned len = be16(b);
        if (len < 2)
            return -1;
        int sof = c >= 0xC0 && c <= 0xCF && c != 0xC4 && c != 0xC8 && c != 0xCC;
        if (sof) {
            if (len < 7 || fread(b, 1, 5, f) != 5)
                return -1;
            *h = (int)be16(b + 1);
            *w = (int)be16(b + 3);
            return 0;
        }
        if (fseek(f, (long)len - 2, SEEK_CUR) != 0)
            return -1;
        if (fgetc(f) != 0xFF)                 /* the next marker must follow */
            return -1;
    }
}

int image_header(const char *path, img_format *format, int *width, int *height)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return -1;
    unsigned char b[24];
    int rc = -1;
    size_t got = fread(b, 1, sizeof b, f);
    static const unsigned char png_sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    if (got == sizeof b && memcmp(b, png_sig, 8) == 0 && memcmp(b + 12, "IHDR", 4) == 0) {
        unsigned long w = be32(b + 16), h = be32(b + 20);
        if (w >= 1 && h >= 1 && w <= 0x7FFFFFFFUL && h <= 0x7FFFFFFFUL) {
            *format = IMG_PNG;
            *width = (int)w;
            *height = (int)h;
            rc = 0;
        }
    } else if (got >= 3 && b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF) {
        if (fseek(f, 2, SEEK_SET) == 0 && jpeg_size(f, width, height) == 0 && *width > 0 && *height > 0) {
            *format = IMG_JPEG;
            rc = 0;
        }
    }
    fclose(f);
    return rc;
}

int save_image(gdImagePtr im, const char *path, int jpeg_quality)
{
    img_format fmt = format_from_name(path);
    if (im == NULL || fmt == IMG_UNKNOWN)
        return -1;
    FILE *out = fopen(path, "wb");            /* "wb": never "w" for image data */
    if (out == NULL)
        return -1;
    if (fmt == IMG_PNG)
        gdImagePng(im, out);
    else
        gdImageJpeg(im, out, jpeg_quality);
    int failed = ferror(out);
    if (fclose(out) != 0)
        failed = 1;
    return failed ? -1 : 0;
}

/* ---- Thumbnails ---- */

void fit_within(int w, int h, int max_side, int *out_w, int *out_h)
{
    if (w <= max_side && h <= max_side) {
        *out_w = w;
        *out_h = h;
        return;
    }
    long long a = w >= h ? w : h, b = w >= h ? h : w;
    int small_side = (int)((b * max_side + a / 2) / a);   /* round to nearest */
    if (small_side < 1)
        small_side = 1;
    *out_w = w >= h ? max_side : small_side;
    *out_h = w >= h ? small_side : max_side;
}

gdImagePtr make_thumbnail(gdImagePtr src, int max_side)
{
    int tw, th;
    fit_within(gdImageSX(src), gdImageSY(src), max_side, &tw, &th);
    gdImagePtr dst = gdImageCreateTrueColor(tw, th);
    if (dst == NULL)
        return NULL;
    gdImageAlphaBlending(dst, 0);             /* keep transparency in PNGs */
    gdImageSaveAlpha(dst, 1);
    gdImageCopyResampled(dst, src, 0, 0, 0, 0, tw, th, gdImageSX(src), gdImageSY(src));
    return dst;
}

/* ---- Identicons ---- */

uint32_t fnv1a32(const char *s)
{
    uint32_t h = 2166136261u;
    for (; *s != '\0'; s++) {
        h ^= (unsigned char)*s;
        h *= 16777619u;
    }
    return h;
}

/* Bits 0-14 of the hash fill the left three columns; columns 3 and 4
   mirror columns 1 and 0. The color comes from a second mix of the hash. */
void identicon_cells(const char *name, int cells[5][5], int *r, int *g, int *b)
{
    uint32_t h = fnv1a32(name);
    for (int row = 0; row < 5; row++)
        for (int col = 0; col < 3; col++) {
            int on = (int)(h >> (row * 3 + col) & 1u);
            cells[row][col] = on;
            cells[row][4 - col] = on;
        }
    uint32_t c = h * 2654435761u;
    *r = 40 + (int)(c >> 24 & 0xFF) % 160;
    *g = 40 + (int)(c >> 16 & 0xFF) % 160;
    *b = 40 + (int)(c >> 8 & 0xFF) % 160;
}

gdImagePtr draw_identicon(const char *name, int cell_px)
{
    int cells[5][5], r, g, b;
    identicon_cells(name, cells, &r, &g, &b);
    int margin = cell_px / 2, size = 5 * cell_px + 2 * margin;
    gdImagePtr im = gdImageCreateTrueColor(size, size);
    if (im == NULL)
        return NULL;
    gdImageFilledRectangle(im, 0, 0, size - 1, size - 1, gdTrueColor(240, 240, 240));
    int fg = gdTrueColor(r, g, b);
    for (int row = 0; row < 5; row++)
        for (int col = 0; col < 5; col++)
            if (cells[row][col]) {
                int x = margin + col * cell_px, y = margin + row * cell_px;
                gdImageFilledRectangle(im, x, y, x + cell_px - 1, y + cell_px - 1, fg);
            }
    return im;
}

/* ---- Bar charts ---- */

int read_bars(const char *path, bar *bars, int max_bars)
{
    FILE *in = fopen(path, "r");
    if (in == NULL)
        return -1;
    char line[128];
    int n = 0;
    while (n < max_bars && fgets(line, sizeof line, in) != NULL)
        if (line[0] != '#' && sscanf(line, "%7s %ld", bars[n].label, &bars[n].value) == 2
            && bars[n].value >= 0)
            n++;
    fclose(in);
    return n;
}

int bar_height_px(long value, long max, int plot_h)
{
    if (max <= 0)
        return 0;
    return (int)((value * plot_h + max / 2) / max);
}

static void centered_string(gdImagePtr im, gdFontPtr font, int cx, int y, const char *s, int color)
{
    int w = (int)strlen(s) * font->w;
    gdImageString(im, font, cx - w / 2, y, (unsigned char *)s, color);
}

gdImagePtr draw_bar_chart(const bar *bars, int n, int width, int height,
                          const char *title, const char *font_path, int *used_freetype)
{
    if (n < 1 || width < CHART_LEFT + CHART_RIGHT + n || height < CHART_TOP + CHART_BOTTOM + 10)
        return NULL;
    gdImagePtr im = gdImageCreateTrueColor(width, height);
    if (im == NULL)
        return NULL;
    int white = gdTrueColor(255, 255, 255), dark = gdTrueColor(0x1F, 0x29, 0x37);
    int grey = gdTrueColor(0x6B, 0x72, 0x80), grid = gdTrueColor(0xE5, 0xE7, 0xEB);
    int blue = gdTrueColor(CHART_BAR_COLOR >> 16, CHART_BAR_COLOR >> 8 & 0xFF, CHART_BAR_COLOR & 0xFF);
    gdImageFilledRectangle(im, 0, 0, width - 1, height - 1, white);

    int x0 = CHART_LEFT, x1 = width - CHART_RIGHT, base = height - CHART_BOTTOM;
    int plot_h = base - CHART_TOP;
    long max = 0;
    for (int i = 0; i < n; i++)
        if (bars[i].value > max)
            max = bars[i].value;

    /* Grid lines and their values at 0, 1/4, 1/2, 3/4 and the maximum. */
    char text[32];
    for (int k = 0; k <= 4; k++) {
        int y = base - plot_h * k / 4;
        if (k > 0)
            gdImageLine(im, x0 + 1, y, x1, y, grid);
        snprintf(text, sizeof text, "%ld", max * k / 4);
        gdFontPtr f = gdFontGetSmall();
        gdImageString(im, f, x0 - 6 - (int)strlen(text) * f->w, y - f->h / 2, (unsigned char *)text, grey);
    }

    int slot = (x1 - x0) / n, bw = slot * 3 / 5;
    for (int i = 0; i < n; i++) {
        int bh = bar_height_px(bars[i].value, max, plot_h);
        int bx = x0 + i * slot + (slot - bw) / 2, cx = bx + bw / 2;
        if (bh > 0)
            gdImageFilledRectangle(im, bx, base - bh, bx + bw - 1, base - 1, blue);
        snprintf(text, sizeof text, "%ld", bars[i].value);
        centered_string(im, gdFontGetTiny(), cx, base - bh - 12, text, dark);
        centered_string(im, gdFontGetSmall(), cx, base + 8, bars[i].label, dark);
    }
    gdImageLine(im, x0, CHART_TOP, x0, base, dark);   /* axes */
    gdImageLine(im, x0, base, x1, base, dark);

    int ft = 0;
    if (title != NULL && font_path != NULL) {
        int box[8];
        char *err = gdImageStringFT(NULL, box, dark, (char *)font_path, 14.0, 0.0, 0, 0, (char *)title);
        if (err == NULL) {                    /* measured: now draw it centered */
            int tw = box[2] - box[0];
            err = gdImageStringFT(im, box, dark, (char *)font_path, 14.0, 0.0, (width - tw) / 2, 26, (char *)title);
            ft = err == NULL;
        }
    }
    if (title != NULL && !ft)
        centered_string(im, gdFontGetLarge(), width / 2, 12, title, dark);
    if (used_freetype != NULL)
        *used_freetype = ft;
    return im;
}
