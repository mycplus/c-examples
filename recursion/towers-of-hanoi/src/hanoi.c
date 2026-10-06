/* hanoi.c - Towers of Hanoi solvers in C11. */
#include "hanoi.h"

void hanoi_recursive(unsigned n, char source, char target, char spare,
                     hanoi_visit visit, void *ctx)
{
    if (n == 0)
        return;
    hanoi_recursive(n - 1, source, spare, target, visit, ctx);
    visit(n, source, target, ctx);
    hanoi_recursive(n - 1, spare, target, source, visit, ctx);
}

/* Number of trailing zero bits in k, for k != 0. A loop rather than a
 * compiler builtin, so the file builds unchanged with MSVC. */
static unsigned trailing_zeros(uint64_t k)
{
    unsigned z = 0;
    while ((k & 1u) == 0) {
        k >>= 1;
        ++z;
    }
    return z;
}

bool hanoi_move_at(unsigned n, uint64_t k, char source, char target, char spare,
                   unsigned *disk, char *from, char *to)
{
    uint64_t last;
    if (!hanoi_move_count(n, &last) || k == 0 || k > last)
        return false;

    /* With the pegs indexed 0, 1, 2, the bit formula below moves the tower
     * from index 0 to index 2 when n is odd and to index 1 when n is even,
     * so the order of target and spare depends on the parity of n. */
    const char peg[3] = { source,
                          n % 2 == 1 ? spare : target,
                          n % 2 == 1 ? target : spare };

    /* The textbook form ((k | (k - 1)) + 1) % 3 wraps to 0 when
     * k | (k - 1) == UINT64_MAX, which happens for n == 64. Reducing
     * modulo 3 before adding 1 cannot overflow. */
    *disk = trailing_zeros(k) + 1;
    *from = peg[(k & (k - 1)) % 3];
    *to = peg[((k | (k - 1)) % 3 + 1) % 3];
    return true;
}

bool hanoi_iterative(unsigned n, char source, char target, char spare,
                     hanoi_visit visit, void *ctx)
{
    uint64_t last;
    if (!hanoi_move_count(n, &last))
        return false;
    if (last == 0)
        return true;
    for (uint64_t k = 1;; ++k) {
        unsigned disk;
        char from, to;
        hanoi_move_at(n, k, source, target, spare, &disk, &from, &to);
        visit(disk, from, to, ctx);
        if (k == last)        /* not k <= last: last may be UINT64_MAX */
            break;
    }
    return true;
}

bool hanoi_move_count(unsigned n, uint64_t *out)
{
    if (n > HANOI_MAX_DISKS)
        return false;
    /* Shifting a 64-bit value by 64 is undefined, so n == 64 is a case
     * of its own. */
    *out = n == 64 ? UINT64_MAX : (UINT64_C(1) << n) - 1;
    return true;
}
