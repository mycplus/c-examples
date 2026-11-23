/* imgtools: small helpers on top of LibGD, used by the example programs
   and tested by tests/test_imgtools.c. */
#ifndef IMGTOOLS_H
#define IMGTOOLS_H

#include <gd.h>
#include <stdint.h>

typedef enum { IMG_UNKNOWN, IMG_PNG, IMG_JPEG } img_format;

/* The format named by a file's extension (.png, .jpg, .jpeg; any case). */
img_format format_from_name(const char *path);

/* The format found in a file's first bytes, and its width and height, read
   from the header without decoding the pixels. Returns 0 on success. */
int image_header(const char *path, img_format *format, int *width, int *height);

/* Writes im to path as PNG or JPEG, chosen by the extension. The file is
   opened in binary mode. jpeg_quality is 0-100, or -1 for the default.
   Returns 0 on success, -1 on any error. */
int save_image(gdImagePtr im, const char *path, int jpeg_quality);

/* The largest size that fits in a max_side x max_side box and keeps the
   w:h aspect ratio. Never enlarges, and never returns a side below 1. */
void fit_within(int w, int h, int max_side, int *out_w, int *out_h);

/* A resampled true-color copy of src that fits within max_side. */
gdImagePtr make_thumbnail(gdImagePtr src, int max_side);

/* ---- Identicons: a symmetric 5 x 5 avatar drawn from a name's hash ---- */
uint32_t fnv1a32(const char *s);
void identicon_cells(const char *name, int cells[5][5], int *r, int *g, int *b);
gdImagePtr draw_identicon(const char *name, int cell_px);

/* ---- Bar charts ---- */
typedef struct {
    char label[8];
    long value;
} bar;

enum { CHART_LEFT = 60, CHART_RIGHT = 20, CHART_TOP = 40, CHART_BOTTOM = 40 };
#define CHART_BAR_COLOR 0x1A6FD4

/* Reads "label value" lines (lines starting with # are skipped).
   Returns the number of bars read, or -1 if the file cannot be opened. */
int read_bars(const char *path, bar *bars, int max_bars);

/* Height in pixels of a bar for value, scaled so that max fills plot_h,
   rounded to the nearest pixel. */
int bar_height_px(long value, long max, int plot_h);

/* Draws a bar chart. If font_path names a TrueType font and LibGD was built
   with FreeType, the title uses it; otherwise a built-in font is used.
   *used_freetype (if not NULL) says which. */
gdImagePtr draw_bar_chart(const bar *bars, int n, int width, int height,
                          const char *title, const char *font_path, int *used_freetype);

#endif
