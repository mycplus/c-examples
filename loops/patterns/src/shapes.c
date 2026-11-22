/* shapes: a second way to draw patterns. Instead of counting spaces and
   stars per row, test every cell of a grid: the shape is a rule that
   says whether cell (x, y) is filled. x and y are measured from the
   center, so symmetric shapes have short rules.

   usage: shapes NAME SIZE     NAME: diamond, hollow-diamond, x, plus, circle, triangle */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*rule)(int x, int y, int r);

static int diamond(int x, int y, int r)        { return abs(x) + abs(y) <= r; }
static int hollow_diamond(int x, int y, int r) { return abs(x) + abs(y) == r; }
static int cross_x(int x, int y, int r)        { (void)r; return abs(x) == abs(y); }
static int plus(int x, int y, int r)           { (void)r; return x == 0 || y == 0; }
static int circle(int x, int y, int r)         { return x * x + y * y <= r * r + r; }
static int triangle(int x, int y, int r)       { return y <= 0 && abs(x) <= y + r; }

/* Prints the cells from -r to r in both directions. A row stops at its
   rightmost filled cell, so no line ends in spaces; rows with no filled
   cell are not printed at all. */
static void draw(rule filled, int r)
{
    for (int y = -r; y <= r; y++) {
        int last = -r - 1;
        for (int x = -r; x <= r; x++)
            if (filled(x, y, r))
                last = x;
        if (last < -r)
            continue;
        for (int x = -r; x <= last; x++)
            putchar(filled(x, y, r) ? '*' : ' ');
        putchar('\n');
    }
}

int main(int argc, char **argv)
{
    static const struct { const char *name; rule f; } shapes[] = {
        {"diamond", diamond}, {"hollow-diamond", hollow_diamond}, {"x", cross_x},
        {"plus", plus}, {"circle", circle}, {"triangle", triangle},
    };
    int r = argc == 3 ? atoi(argv[2]) : 0;
    for (size_t i = 0; argc == 3 && i < sizeof shapes / sizeof shapes[0]; i++)
        if (strcmp(argv[1], shapes[i].name) == 0 && r >= 1 && r <= 30) {
            draw(shapes[i].f, r);
            return 0;
        }
    fprintf(stderr, "usage: shapes diamond|hollow-diamond|x|plus|circle|triangle SIZE (1-30)\n");
    return 2;
}
