/* palette_fade.c - an 8-bit indexed image fades to black by changing
 * only its 256-entry palette; the pixel bytes are never written. */
#include <stdint.h>
#include <stdio.h>

typedef struct { uint8_t r, g, b; } rgb;

enum { W = 4, H = 2 };

static void show(const uint8_t pixels[H][W], const rgb palette[256], int step)
{
    printf("step %d:", step);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            rgb c = palette[pixels[y][x]];
            printf(" (%3u,%3u,%3u)", c.r, c.g, c.b);
            if (x == W - 1 && y == 0) printf("\n       ");
        }
    putchar('\n');
}

static unsigned checksum(const uint8_t pixels[H][W])
{
    unsigned sum = 0;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            sum = sum * 31u + pixels[y][x];
    return sum;
}

int main(void)
{
    const uint8_t pixels[H][W] = { { 1, 2, 3, 1 }, { 3, 2, 1, 0 } };
    rgb base[256] = { [0] = {0, 0, 0}, [1] = {255, 0, 0},
                      [2] = {0, 200, 0}, [3] = {40, 80, 255} };
    rgb palette[256];

    unsigned before = checksum(pixels);
    for (int step = 0; step <= 4; step += 2) {
        for (int i = 0; i < 256; i++) {          /* scale every entry */
            palette[i].r = (uint8_t)(base[i].r * (4 - step) / 4);
            palette[i].g = (uint8_t)(base[i].g * (4 - step) / 4);
            palette[i].b = (uint8_t)(base[i].b * (4 - step) / 4);
        }
        show(pixels, palette, step);
    }
    printf("pixel checksum before %u, after %u\n", before, checksum(pixels));
    return 0;
}
