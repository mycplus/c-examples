/* pitch.c - why a surface's row stride (pitch) is not width * bytes-per-pixel.
 * Draws a diagonal into a padded 6x6 surface, once with the pitch and once
 * with the width, then prints what the display would show. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { WIDTH = 6, HEIGHT = 6, BPP = 4, PITCH = 32 }; /* 32 > 6 * 4 = 24 */

static void show(const char *title, const uint8_t *surface)
{
    printf("%s\n", title);
    for (int y = 0; y < HEIGHT; y++) {
        printf("  ");
        for (int x = 0; x < WIDTH; x++)          /* the display reads with PITCH */
            putchar(surface[(size_t)y * PITCH + (size_t)x * BPP] ? '#' : '.');
        putchar('\n');
    }
}

int main(void)
{
    static uint8_t surface[HEIGHT * PITCH];

    memset(surface, 0, sizeof surface);
    for (int i = 0; i < WIDTH; i++)
        surface[(size_t)i * PITCH + (size_t)i * BPP] = 0xFF;
    show("Row offset = y * pitch:", surface);

    memset(surface, 0, sizeof surface);
    for (int i = 0; i < WIDTH; i++)
        surface[(size_t)i * (WIDTH * BPP) + (size_t)i * BPP] = 0xFF;
    show("Row offset = y * width * 4:", surface);
    return 0;
}
