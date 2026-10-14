/* trace.c - parses the two trace formats described in trace.h. */
#include "trace.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

void trace_init(trace_reader *r, FILE *in)
{
    memset(r, 0, sizeof *r);
    r->in = in;
}

/* Parses a hexadecimal number with an optional 0x prefix. On success stores
 * it, advances *p past it and returns true. Rejects values over 64 bits. */
static bool parse_hex(const char **p, uint64_t *out)
{
    const char *s = *p;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        s += 2;
    if (!isxdigit((unsigned char)*s))
        return false;
    char *end;
    errno = 0;
    const unsigned long long v = strtoull(s, &end, 16);
    if (errno == ERANGE)
        return false;
    *out = (uint64_t)v;
    *p = end;
    return true;
}

static const char *skip_spaces(const char *p)
{
    while (*p == ' ' || *p == '\t')
        p++;
    return p;
}

static bool only_spaces_left(const char *p)
{
    p = skip_spaces(p);
    return *p == '\0' || *p == '\n' || *p == '\r';
}

int trace_next(trace_reader *r, trace_access *out)
{
    if (r->pending_store) {
        r->pending_store = false;
        *out = r->pending;
        return 1;
    }
    char buf[256];
    while (fgets(buf, sizeof buf, r->in) != NULL) {
        r->line++;
        if (strchr(buf, '\n') == NULL && !feof(r->in))
            return -1;                              /* line too long */
        const char *p = skip_spaces(buf);
        if (*p == '\0' || *p == '\n' || *p == '\r' || *p == '=')
            continue;
        if (*p == '#') {
            if (strncmp(p, "#eof", 4) == 0)
                return 0;
            continue;
        }

        /* Valgrind Lackey: kind, space, hex address, comma, decimal size. */
        if ((*p == 'I' || *p == 'L' || *p == 'S' || *p == 'M') && (p[1] == ' ' || p[1] == '\t')) {
            const char kind = *p;
            const char *q = skip_spaces(p + 1);
            uint64_t addr = 0;
            if (!parse_hex(&q, &addr) || *q != ',')
                return -1;
            if (!isdigit((unsigned char)q[1]))
                return -1;
            char *end;
            errno = 0;
            const unsigned long long size = strtoull(q + 1, &end, 10);
            if (errno == ERANGE || size == 0 || !only_spaces_left(end))
                return -1;
            if (kind == 'I')
                continue;
            out->addr = addr;
            out->size = (uint64_t)size;
            out->is_write = kind == 'S';
            if (kind == 'M') {                      /* load now, store next call */
                r->pending = *out;
                r->pending.is_write = true;
                r->pending_store = true;
            }
            return 1;
        }

        /* [PC:] R|W address */
        const char *colon = strchr(p, ':');
        if (colon != NULL)
            p = skip_spaces(colon + 1);
        if ((*p != 'R' && *p != 'W') || (p[1] != ' ' && p[1] != '\t'))
            return -1;
        out->is_write = *p == 'W';
        p = skip_spaces(p + 1);
        if (!parse_hex(&p, &out->addr) || !only_spaces_left(p))
            return -1;
        out->size = 1;
        return 1;
    }
    return 0;
}
