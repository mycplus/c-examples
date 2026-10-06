/* hanoi.h - Towers of Hanoi: recursive and iterative solvers and the
 * move count, for C11. */
#ifndef MYCPLUS_HANOI_H
#define MYCPLUS_HANOI_H

#include <stdbool.h>
#include <stdint.h>

/* Largest disk count the iterative solver and hanoi_move_count accept:
 * 2^64 - 1 moves is the largest count a uint64_t can hold. */
#define HANOI_MAX_DISKS 64u

/* Called once per move: `disk` runs from 1 (smallest) to n (largest). */
typedef void (*hanoi_visit)(unsigned disk, char source, char target, void *ctx);

/* Recursive solution: moves n disks from `source` to `target` using
 * `spare`, calling visit() for each move. Recursion depth is n + 1. */
void hanoi_recursive(unsigned n, char source, char target, char spare,
                     hanoi_visit visit, void *ctx);

/* Iterative solution with no stack: move k (1-based) moves disk
 * ctz(k) + 1, and its pegs follow from the bits of k. Produces the same
 * sequence as hanoi_recursive. Returns false, making no calls, if
 * n > HANOI_MAX_DISKS. */
bool hanoi_iterative(unsigned n, char source, char target, char spare,
                     hanoi_visit visit, void *ctx);

/* Computes move k (1 <= k <= 2^n - 1) of the n-disk solution directly,
 * without generating the moves before it. Returns false if n or k is out
 * of range. */
bool hanoi_move_at(unsigned n, uint64_t k, char source, char target, char spare,
                   unsigned *disk, char *from, char *to);

/* Stores 2^n - 1 in *out. Returns false, leaving *out unchanged, if
 * n > HANOI_MAX_DISKS. */
bool hanoi_move_count(unsigned n, uint64_t *out);

#endif
