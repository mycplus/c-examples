/* test_hanoi.c - checks the solvers by simulating the pegs. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hanoi.h"

static int failures;

static void check(int ok, const char *what, unsigned n)
{
    if (!ok) {
        fprintf(stderr, "CHECK failed: %s (n=%u)\n", what, n);
        failures++;
    }
}
#define CHECK(cond, n) check((cond) != 0, #cond, (n))

/* ---- A simulator: three pegs, each a stack of disk numbers. ---- */
#define MAX_SIM 20u

typedef struct {
    unsigned peg[3][MAX_SIM];
    unsigned height[3];
    unsigned long long moves;
    int illegal;            /* set on the first illegal move */
} Sim;

static int index_of(char peg)
{
    return peg == 'A' ? 0 : peg == 'B' ? 1 : peg == 'C' ? 2 : -1;
}

static void sim_init(Sim *s, unsigned n)
{
    memset(s, 0, sizeof *s);
    for (unsigned i = 0; i < n; ++i)
        s->peg[0][i] = n - i;           /* largest at the bottom */
    s->height[0] = n;
}

static void sim_move(unsigned disk, char source, char target, void *ctx)
{
    Sim *s = ctx;
    int f = index_of(source), t = index_of(target);
    s->moves++;
    if (s->illegal)
        return;
    if (f < 0 || t < 0 || f == t || s->height[f] == 0 ||
        s->peg[f][s->height[f] - 1] != disk ||                 /* not the top disk */
        (s->height[t] > 0 && s->peg[t][s->height[t] - 1] < disk)) { /* onto smaller */
        s->illegal = 1;
        return;
    }
    s->peg[t][s->height[t]++] = s->peg[f][--s->height[f]];
}

static int sim_solved(const Sim *s, unsigned n)
{
    if (s->illegal || s->height[0] != 0 || s->height[1] != 0 || s->height[2] != n)
        return 0;
    for (unsigned i = 0; i < n; ++i)
        if (s->peg[2][i] != n - i)
            return 0;
    return 1;
}

/* ---- Record a move sequence so two solvers can be compared. ---- */
typedef struct {
    unsigned char disk;
    char from, to;
} Move;

typedef struct {
    Move *m;
    size_t len;
} Log;

static void log_move(unsigned disk, char source, char target, void *ctx)
{
    Log *l = ctx;
    l->m[l->len].disk = (unsigned char)disk;
    l->m[l->len].from = source;
    l->m[l->len].to = target;
    l->len++;
}

/* ---- An independent reference for move k: descend the recursion. ----
 * The middle move (k == 2^(n-1)) moves disk n; earlier moves belong to the
 * first n-1 subproblem, later ones to the second. O(n), no bit tricks. */
static void reference_move(unsigned n, uint64_t k, char s, char t, char v,
                           unsigned *disk, char *from, char *to)
{
    for (;;) {
        uint64_t mid = UINT64_C(1) << (n - 1);
        if (k == mid) {
            *disk = n;
            *from = s;
            *to = t;
            return;
        }
        char ns, nt, nv;
        if (k < mid) {
            ns = s; nt = v; nv = t;
        } else {
            k -= mid;
            ns = v; nt = t; nv = s;
        }
        s = ns; t = nt; v = nv;
        --n;
    }
}

static uint64_t state = UINT64_C(0x9E3779B97F4A7C15);

static uint64_t next_rand(void)
{
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
}

int main(void)
{
    /* Both solvers, every n from 0 to 20: legal moves, solved state,
     * 2^n - 1 moves, and identical sequences. */
    Move *a = malloc(sizeof *a * ((size_t)1 << MAX_SIM));
    Move *b = malloc(sizeof *b * ((size_t)1 << MAX_SIM));
    if (a == NULL || b == NULL) {
        fputs("out of memory\n", stderr);
        return EXIT_FAILURE;
    }
    for (unsigned n = 0; n <= MAX_SIM; ++n) {
        uint64_t expected;
        CHECK(hanoi_move_count(n, &expected), n);

        Sim s;
        sim_init(&s, n);
        hanoi_recursive(n, 'A', 'C', 'B', sim_move, &s);
        CHECK(sim_solved(&s, n), n);
        CHECK(s.moves == expected, n);

        sim_init(&s, n);
        CHECK(hanoi_iterative(n, 'A', 'C', 'B', sim_move, &s), n);
        CHECK(sim_solved(&s, n), n);
        CHECK(s.moves == expected, n);

        Log la = { a, 0 }, lb = { b, 0 };
        hanoi_recursive(n, 'A', 'C', 'B', log_move, &la);
        hanoi_iterative(n, 'A', 'C', 'B', log_move, &lb);
        CHECK(la.len == lb.len && memcmp(a, b, la.len * sizeof *a) == 0, n);

        /* Every move of the recursive sequence agrees with hanoi_move_at. */
        int all_match = 1;
        for (size_t i = 0; i < la.len; ++i) {
            unsigned d;
            char f, t;
            if (!hanoi_move_at(n, i + 1, 'A', 'C', 'B', &d, &f, &t) ||
                d != a[i].disk || f != a[i].from || t != a[i].to)
                all_match = 0;
        }
        CHECK(all_match, n);
    }
    free(a);
    free(b);

    /* Other peg labellings: move from C to A using B. */
    {
        Sim s;
        sim_init(&s, 7);
        /* Relabel so the simulator's A,B,C are the solver's C,B,A. */
        Move buf[127];
        Log lc = { buf, 0 };
        hanoi_recursive(7, 'C', 'A', 'B', log_move, &lc);
        for (size_t i = 0; i < lc.len; ++i) {
            char f = (char)('A' + 'C' - buf[i].from);
            char t = (char)('A' + 'C' - buf[i].to);
            sim_move(buf[i].disk, f, t, &s);
        }
        CHECK(sim_solved(&s, 7), 7u);
    }

    /* Move counts at the 64-bit boundary. */
    {
        uint64_t c = 0;
        CHECK(hanoi_move_count(63, &c) && c == UINT64_C(9223372036854775807), 63u);
        CHECK(hanoi_move_count(64, &c) && c == UINT64_MAX, 64u);
        c = 12345;
        CHECK(!hanoi_move_count(65, &c) && c == 12345, 65u);
        CHECK(!hanoi_iterative(65, 'A', 'C', 'B', sim_move, NULL), 65u);
    }

    /* hanoi_move_at against the descent reference for n = 64, including
     * the last move and moves where k | (k - 1) == UINT64_MAX. */
    {
        uint64_t ks[2000];
        size_t nk = 0;
        ks[nk++] = 1;
        ks[nk++] = UINT64_MAX;
        ks[nk++] = UINT64_C(1) << 63;
        for (unsigned j = 0; j < 64; ++j)
            ks[nk++] = UINT64_MAX - ((UINT64_C(1) << j) - 1); /* 2^64 - 2^j */
        while (nk < sizeof ks / sizeof ks[0]) {
            uint64_t k = next_rand();
            ks[nk++] = k == 0 ? 1 : k;
        }
        int all_match = 1;
        for (size_t i = 0; i < nk; ++i) {
            unsigned d1, d2;
            char f1, f2, t1, t2;
            reference_move(64, ks[i], 'A', 'C', 'B', &d1, &f1, &t1);
            if (!hanoi_move_at(64, ks[i], 'A', 'C', 'B', &d2, &f2, &t2) ||
                d1 != d2 || f1 != f2 || t1 != t2) {
                fprintf(stderr, "k=%llu: reference %u %c->%c, got %u %c->%c\n",
                        (unsigned long long)ks[i], d1, f1, t1, d2, f2, t2);
                all_match = 0;
            }
        }
        CHECK(all_match, 64u);

        /* Out-of-range k and n. */
        unsigned d;
        char f, t;
        CHECK(!hanoi_move_at(3, 0, 'A', 'C', 'B', &d, &f, &t), 3u);
        CHECK(!hanoi_move_at(3, 8, 'A', 'C', 'B', &d, &f, &t), 3u);
        CHECK(!hanoi_move_at(0, 1, 'A', 'C', 'B', &d, &f, &t), 0u);
        CHECK(!hanoi_move_at(65, 1, 'A', 'C', 'B', &d, &f, &t), 65u);
    }

    if (failures != 0) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    puts("all tests passed");
    return EXIT_SUCCESS;
}
