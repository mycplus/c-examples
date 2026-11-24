/* ledger.h - the ledger file: one line per entry, only ever appended to.
 *
 * Line 1 is the header; every later line is one entry, its fields
 * separated by '|', for example:
 *     # MYCPLUS bank ledger v1
 *     1|2026-12-01|OPEN|100001|S|150000|Ana Lopez|12 Harbour Road, Springfield
 *     4|2026-12-01|DEP|100001|25075|175075
 * Money is written in whole cents with no decimal point. The file is for
 * the program, so it holds plain numbers with nothing to parse but digits;
 * the screen is for people, and only the screen adds commas and a point. */
#ifndef LEDGER_H
#define LEDGER_H

#include <stddef.h>

#include "bank.h"

#define LEDGER_HEADER "# MYCPLUS bank ledger v1"
#define LEDGER_LINE   256
#define REASON_SIZE   160

/* LOAD_NO_FILE: the file does not exist yet (a new bank).
 * LOAD_CANNOT_OPEN: it exists but cannot be opened (no permission, ...).
 * LOAD_ERROR: it opened, but a line breaks a rule; nothing may be used. */
typedef enum { LOAD_OK, LOAD_NO_FILE, LOAD_CANNOT_OPEN, LOAD_ERROR } LoadResult;

int  entry_to_line(const Entry *entry, char *line, size_t size);   /* no newline */
int  line_to_entry(char *line, Entry *entry, char *why, size_t why_size);   /* splits at '|' */
void rule_reason(const Bank *bank, const Entry *entry, Rule rule, char *why, size_t why_size);
LoadResult ledger_load(Bank *bank, const char *path, char *error, size_t error_size);
int  ledger_append(const char *path, const Entry *entry, int write_header, char *error, size_t error_size);

#endif
