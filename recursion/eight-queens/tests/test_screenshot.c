/* Loads a screenshot made by `queens_view 8 --screenshot 1 FILE` and checks
 * that queen green appears in exactly the eight squares of solution 1. */
#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

enum { SQUARE = 64, MARGIN = 24, N = 8 };

int main(int argc, char **argv)
{
    static const int solution[N] = {0, 4, 7, 5, 2, 6, 1, 3};
    if (argc != 2)
        return 2;
    SDL_Surface *loaded = SDL_LoadBMP(argv[1]);
    SDL_Surface *s = loaded ? SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_RGBA32, 0) : NULL;
    if (s == NULL) {
        fprintf(stderr, "cannot load %s: %s\n", argv[1], SDL_GetError());
        return 1;
    }
    int failures = 0;
    if (s->w != 2 * MARGIN + N * SQUARE || s->h != s->w) {
        fprintf(stderr, "unexpected size %dx%d\n", s->w, s->h);
        failures++;
    }
    for (int row = 0; row < N && failures == 0; row++)
        for (int c = 0; c < N; c++) {
            int green = 0;
            for (int y = 0; y < SQUARE; y++)
                for (int x = 0; x < SQUARE; x++) {
                    const Uint8 *p = (const Uint8 *)s->pixels +
                                     (MARGIN + row * SQUARE + y) * s->pitch +
                                     (MARGIN + c * SQUARE + x) * 4;
                    green += p[0] == 0x46 && p[1] == 0x7f && p[2] == 0x40;
                }
            const int want = solution[row] == c;
            if ((green > 100) != want) {
                fprintf(stderr, "square (%d, %d): %d green pixels, queen expected: %d\n",
                        row, c, green, want);
                failures++;
            }
        }
    SDL_FreeSurface(s);
    SDL_FreeSurface(loaded);
    if (failures == 0)
        printf("screenshot: queens drawn in the right squares\n");
    return failures == 0 ? 0 : 1;
}
