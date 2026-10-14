/* queens_view.c - browse every solution in an SDL2 window.
 * Usage: queens_view [N]                     (N is 4 to 12, default 8)
 *        queens_view N --screenshot K FILE   (draw solution K to a BMP and exit)
 * Keys: Right/Space next, Left previous, Home first, Esc or Q quit. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "queens.h"

enum { SQUARE = 64, MARGIN = 24 };

typedef struct {
    queens_board *items;
    size_t count, cap;
} solution_list;

static bool collect(const queens_board *b, void *ctx)
{
    solution_list *list = ctx;
    if (list->count == list->cap) {
        const size_t cap = list->cap ? list->cap * 2 : 64;
        queens_board *grown = realloc(list->items, cap * sizeof *grown);
        if (grown == NULL)
            return false;                      /* stop the search; count stays valid */
        list->items = grown;
        list->cap = cap;
    }
    list->items[list->count++] = *b;
    return true;
}

static void fill_circle(SDL_Renderer *r, int cx, int cy, int radius)
{
    for (int dy = -radius; dy <= radius; dy++) {
        int dx = 0;
        while ((dx + 1) * (dx + 1) + dy * dy <= radius * radius)
            dx++;
        SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

/* A queen: a base, a body and a crown of three balls, all from rectangles
 * and circles so no image or font files are needed. */
static void draw_queen(SDL_Renderer *r, int x, int y)
{
    const int cx = x + SQUARE / 2;
    SDL_SetRenderDrawColor(r, 0x46, 0x7f, 0x40, 0xff);             /* green */
    const SDL_Rect base = {x + 14, y + 46, SQUARE - 28, 8};
    const SDL_Rect body = {x + 22, y + 26, SQUARE - 44, 22};
    SDL_RenderFillRect(r, &base);
    SDL_RenderFillRect(r, &body);
    fill_circle(r, cx, y + 26, 11);
    fill_circle(r, cx - 14, y + 16, 5);
    fill_circle(r, cx, y + 11, 5);
    fill_circle(r, cx + 14, y + 16, 5);
}

static void draw_board(SDL_Renderer *r, const queens_board *b)
{
    SDL_SetRenderDrawColor(r, 0xf4, 0xf5, 0xf7, 0xff);
    SDL_RenderClear(r);
    for (int row = 0; row < b->n; row++)
        for (int c = 0; c < b->n; c++) {
            const SDL_Rect sq = {MARGIN + c * SQUARE, MARGIN + row * SQUARE, SQUARE, SQUARE};
            if ((row + c) % 2 == 0)
                SDL_SetRenderDrawColor(r, 0xe5, 0xe7, 0xeb, 0xff);
            else
                SDL_SetRenderDrawColor(r, 0x9c, 0xa3, 0xaf, 0xff);
            SDL_RenderFillRect(r, &sq);
        }
    /* The same SQUARE and MARGIN place the queens, so they cannot drift. */
    for (int row = 0; row < b->n; row++)
        draw_queen(r, MARGIN + b->col[row] * SQUARE, MARGIN + row * SQUARE);
}

static int screenshot(const queens_board *b, const char *path)
{
    const int side = 2 * MARGIN + b->n * SQUARE;
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, side, side, 32,
                                                          SDL_PIXELFORMAT_RGBA32);
    SDL_Renderer *r = surface ? SDL_CreateSoftwareRenderer(surface) : NULL;
    if (r == NULL) {
        fprintf(stderr, "queens_view: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return 1;
    }
    draw_board(r, b);
    SDL_RenderPresent(r);
    const int ok = SDL_SaveBMP(surface, path) == 0;
    if (!ok)
        fprintf(stderr, "queens_view: %s\n", SDL_GetError());
    SDL_DestroyRenderer(r);
    SDL_FreeSurface(surface);
    return ok ? 0 : 1;
}

static void set_title(SDL_Window *w, int n, size_t i, size_t count)
{
    char title[96];
    snprintf(title, sizeof title, "%d queens: solution %zu of %zu", n, i + 1, count);
    SDL_SetWindowTitle(w, title);
}

static int browse(const solution_list *list, int n)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "queens_view: %s\n", SDL_GetError());
        return 1;
    }
    const int side = 2 * MARGIN + n * SQUARE;
    SDL_Window *w = SDL_CreateWindow("queens", SDL_WINDOWPOS_CENTERED,
                                     SDL_WINDOWPOS_CENTERED, side, side, 0);
    SDL_Renderer *r = w ? SDL_CreateRenderer(w, -1, 0) : NULL;
    if (r == NULL) {
        fprintf(stderr, "queens_view: %s\n", SDL_GetError());
        if (w != NULL)
            SDL_DestroyWindow(w);
        SDL_Quit();
        return 1;
    }
    size_t i = 0;
    bool redraw = true, quit = false;
    while (!quit) {
        if (redraw) {
            set_title(w, n, i, list->count);
            draw_board(r, &list->items[i]);
            SDL_RenderPresent(r);
            redraw = false;
        }
        SDL_Event e;
        if (!SDL_WaitEvent(&e))                /* sleeps until there is input */
            break;
        if (e.type == SDL_QUIT)
            quit = true;
        else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_EXPOSED)
            redraw = true;
        else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
            case SDLK_RIGHT: case SDLK_SPACE: i = (i + 1) % list->count; redraw = true; break;
            case SDLK_LEFT: i = (i + list->count - 1) % list->count; redraw = true; break;
            case SDLK_HOME: i = 0; redraw = true; break;
            case SDLK_ESCAPE: case SDLK_q: quit = true; break;
            default: break;
            }
        }
    }
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(w);
    SDL_Quit();
    return 0;
}

static bool parse(const char *s, long low, long high, long *out)
{
    char *end;
    errno = 0;
    const long v = strtol(s, &end, 10);
    if (*s < '0' || *s > '9' || *end != '\0' || errno != 0 || v < low || v > high)
        return false;
    *out = v;
    return true;
}

int main(int argc, char **argv)
{
    long n = 8, k = 0;
    const bool shot = argc == 5 && strcmp(argv[2], "--screenshot") == 0;
    if (!((argc == 1) || (argc == 2 && parse(argv[1], 4, 12, &n)) ||
          (shot && parse(argv[1], 4, 12, &n) && parse(argv[3], 1, 1000000, &k)))) {
        fprintf(stderr, "usage: queens_view [N]   or   queens_view N --screenshot K FILE\n"
                        "       (N is 4 to 12)\n");
        return 2;
    }

    solution_list list = {0};
    queens_stats stats;
    queens_solve((int)n, collect, &list, &stats);
    if (list.count != stats.solutions) {
        fprintf(stderr, "queens_view: out of memory\n");
        free(list.items);
        return 1;
    }

    int status;
    if (shot) {
        if ((unsigned long long)k > list.count) {
            fprintf(stderr, "queens_view: there are only %zu solutions\n", list.count);
            free(list.items);
            return 2;
        }
        status = screenshot(&list.items[k - 1], argv[4]);
    } else {
        status = browse(&list, (int)n);
    }
    free(list.items);
    return status;
}
