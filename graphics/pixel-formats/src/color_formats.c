/* color_formats.c - how a pixel colour is laid out in memory:
 * 32-bit XRGB8888 and 16-bit RGB565. */
#include <stdint.h>
#include <stdio.h>

static uint32_t pack_xrgb8888(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
}

static uint16_t pack_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    /* Keep the top 5, 6 and 5 bits of each 8-bit channel. */
    return (uint16_t)((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3));
}

static void dump_bytes(const char *label, const void *p, size_t n)
{
    const unsigned char *bytes = p;
    printf("%-26s", label);
    for (size_t i = 0; i < n; i++)
        printf(" %02X", bytes[i]);
    putchar('\n');
}

int main(void)
{
    const uint16_t probe = 1;
    printf("This machine is %s-endian\n\n",
           *(const unsigned char *)&probe == 1 ? "little" : "big");

    uint32_t green32 = pack_xrgb8888(0, 255, 0);
    uint16_t green16 = pack_rgb565(0, 255, 0);
    printf("XRGB8888 green value:      0x%08X\n", (unsigned)green32);
    dump_bytes("  bytes in memory:", &green32, sizeof green32);
    printf("RGB565 green value:        0x%04X\n", (unsigned)green16);
    dump_bytes("  bytes in memory:", &green16, sizeof green16);

    /* RGB565 cannot store every 8-bit value exactly. */
    uint16_t grey = pack_rgb565(150, 150, 150);
    unsigned r5 = grey >> 11, g6 = (grey >> 5) & 0x3F, b5 = grey & 0x1F;
    printf("\nRGB(150,150,150) as RGB565 = 0x%04X\n", (unsigned)grey);
    printf("  expanded back to 8 bits:  RGB(%u,%u,%u)\n",
           (r5 << 3) | (r5 >> 2), (g6 << 2) | (g6 >> 4), (b5 << 3) | (b5 >> 2));
    return 0;
}
