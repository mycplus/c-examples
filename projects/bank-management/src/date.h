/* date.h - calendar dates written YYYY-MM-DD.
 * Adapted from the mini projects (personal-diary and birthday-reminder):
 * same behavior, with a date_ prefix and without static. */
#ifndef DATE_H
#define DATE_H

#include <stddef.h>

#define DATE_TEXT_SIZE 11   /* "2026-12-01" plus '\0' */

typedef struct { int year; int month; int day; } Date;

int  date_parse(const char *text, Date *out);    /* "YYYY-MM-DD", years 1900-9999, valid day */
int  date_compare(Date a, Date b);               /* <0, 0, >0 */
void date_format(Date d, char *buf, size_t size);/* "2026-12-01" */
int  date_month_key(Date d);                     /* 202612: year * 100 + month */
void date_format_month(int key, char *buf, size_t size);   /* 202612 -> "2026-12" */
Date date_today(void);                           /* from localtime() */

#endif
