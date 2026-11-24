/* input.h - reading typed answers safely, one whole line at a time.
 * read_line, read_int and read_text come from the mini projects
 * (unit-converter and grade-report) with the same behavior and messages. */
#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>

#include "date.h"
#include "money.h"

#define LINE_SIZE 128   /* longest typed line; longer lines are rejected, not split */

int read_line(char *buf, size_t size);                             /* 0 at end of input */
int read_int(const char *prompt, int lo, int hi, int *value);
int read_text(const char *prompt, char *out, size_t size);          /* trims; rejects empty/too long */
int read_money(const char *prompt, Cents *value);                   /* new */
int read_yes_no(const char *prompt, int *yes);                      /* new: y/yes/n/no, any case */
int read_date_or_today(const char *prompt, Date today, Date *out);  /* new: Enter means today */
void trim(char *text);                                              /* removes spaces at both ends */

#endif
