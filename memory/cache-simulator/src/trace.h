/* trace.h - reads memory traces, one access per call.
 *
 * Two line formats are accepted:
 *   R 0x7ffd5a2c        a read or write of one byte; an optional first
 *   W 0x7ffd5a30        field ending in ':' (a program counter) is ignored
 *    L 1ffefffd40,8     Valgrind Lackey: L load, S store, M modify (a load
 *    S 1ffefffd48,8     then a store); I (instruction fetch) lines are
 *    M 0401d1f0,4       skipped
 * Blank lines, lines starting with '#' or '=', and a final "#eof" are skipped. */
#ifndef TRACE_H
#define TRACE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint64_t addr;
    uint64_t size;
    bool is_write;
} trace_access;

typedef struct {
    FILE *in;
    unsigned long line;     /* number of the line last read */
    bool pending_store;     /* second half of a Lackey M line */
    trace_access pending;
} trace_reader;

void trace_init(trace_reader *r, FILE *in);

/* Returns 1 with *out filled, 0 at end of input, or -1 on a malformed or
 * over-long line (r->line says which). */
int trace_next(trace_reader *r, trace_access *out);

#endif
