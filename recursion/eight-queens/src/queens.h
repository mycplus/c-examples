/* queens.h - the N-queens puzzle by backtracking. */
#ifndef QUEENS_H
#define QUEENS_H

#include <stdbool.h>

enum { QUEENS_MAX = 16 };

/* A placement: col[r] is the column (0..n-1) of the queen in row r. */
typedef struct {
    int n;
    int col[QUEENS_MAX];
} queens_board;

/* Called once per solution; return false to stop the search. */
typedef bool (*queens_visit)(const queens_board *b, void *ctx);

typedef struct {
    unsigned long long solutions;
    unsigned long long squares_tested; /* squares checked for attacks */
    unsigned long long placements;    /* queens put on the board, including ones taken back */
} queens_stats;

/* Finds every solution for an n x n board (1 <= n <= QUEENS_MAX), calling
 * visit for each one if it is not NULL. Returns false if n is out of range. */
bool queens_solve(int n, queens_visit visit, void *ctx, queens_stats *stats);

/* True if b has one queen per row and no two queens attack each other. */
bool queens_valid(const queens_board *b);

/* True if b is the first of its family of up to 8 rotations and
 * reflections, so counting these counts distinct solutions. */
bool queens_canonical(const queens_board *b);

#endif
