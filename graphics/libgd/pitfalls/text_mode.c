/* Pitfall: opening an image file in text mode. A PNG file starts with the
   bytes 89 50 4E 47 0D 0A 1A 0A. Its designers put a CR LF and an LF in
   that signature on purpose, so that a text-mode transfer damages it
   visibly. With fopen(..., "w") on Windows, every 0A byte written becomes
   0D 0A, and the file is no longer a PNG. On Linux and macOS "w" and "wb"
   are the same, so code with this bug works there and fails on Windows. */
#include <gd.h>
#include <stdio.h>

static void try_mode(const char *mode)
{
    const char *path = "mode-test.png";
    gdImagePtr im = gdImageCreateTrueColor(64, 64);
    if (im == NULL)
        return;
    gdImageFilledRectangle(im, 0, 0, 63, 63, gdTrueColor(0x1A, 0x6F, 0xD4));
    FILE *out = fopen(path, mode);
    if (out == NULL) {
        gdImageDestroy(im);
        return;
    }
    gdImagePng(im, out);
    fclose(out);
    gdImageDestroy(im);

    unsigned char sig[8] = {0};
    FILE *in = fopen(path, "rb");
    size_t got = in != NULL ? fread(sig, 1, sizeof sig, in) : 0;
    if (in != NULL)
        fclose(in);
    printf("fopen mode \"%s\":%s first bytes", mode, mode[1] == '\0' ? " " : "");
    for (size_t i = 0; i < got; i++)
        printf(" %02X", sig[i]);

    gdImagePtr back = gdImageCreateFromFile(path);
    printf("\n  read back: %s\n", back != NULL ? "a valid 64x64 PNG" : "not a valid PNG");
    if (back != NULL)
        gdImageDestroy(back);
    remove(path);
}

int main(void)
{
    try_mode("wb");
    try_mode("w");
    return 0;
}
