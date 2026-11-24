/* date.c - checking, comparing and printing dates. */
#define _CRT_SECURE_NO_WARNINGS     /* MSVC: allow the standard localtime() */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "date.h"

#define FIRST_YEAR 1900

/* Returns 1 if year is a leap year, 0 otherwise. A year is a leap year if
 * it is divisible by 4, except years divisible by 100, unless they are
 * also divisible by 400: 2024 and 2000 are leap years, 1900 and 2026 not. */
static int is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

/* Reads a date written YYYY-MM-DD into *out: exactly 10 characters, digits
 * and two dashes in the right places, a year from 1900 to 9999, a month
 * from 1 to 12 and a day that exists in that month (29 February only in a
 * leap year). "2026-1-01" and "2026-01-01x" are rejected.
 * Returns 1 if the text is a real date, otherwise 0 (and *out is unchanged). */
int date_parse(const char *text, Date *out)
{
    static const int days_in_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (strlen(text) != 10 || text[4] != '-' || text[7] != '-')
        return 0;
    for (int i = 0; i < 10; i++)
        if (i != 4 && i != 7 && !isdigit((unsigned char)text[i]))
            return 0;

    /* Every character is a checked digit, so c - '0' gives its value. */
    int year  = (text[0] - '0') * 1000 + (text[1] - '0') * 100 + (text[2] - '0') * 10 + (text[3] - '0');
    int month = (text[5] - '0') * 10 + (text[6] - '0');
    int day   = (text[8] - '0') * 10 + (text[9] - '0');
    if (year < FIRST_YEAR || month < 1 || month > 12)
        return 0;
    int last = days_in_month[month - 1];
    if (month == 2 && is_leap_year(year))
        last = 29;
    if (day < 1 || day > last)
        return 0;
    out->year = year;
    out->month = month;
    out->day = day;
    return 1;
}

/* Compares two dates by year, then month, then day.
 * Returns <0 if a is earlier, 0 if they are the same day, >0 if a is later. */
int date_compare(Date a, Date b)
{
    if (a.year != b.year)
        return a.year - b.year;
    if (a.month != b.month)
        return a.month - b.month;
    return a.day - b.day;
}

/* Writes d into buf as YYYY-MM-DD. buf should hold DATE_TEXT_SIZE characters. */
void date_format(Date d, char *buf, size_t size)
{
    snprintf(buf, size, "%04d-%02d-%02d", d.year, d.month, d.day);
}

/* Returns the month of d as one number, year * 100 + month: 2026-12-31
 * gives 202612. Month keys sort in date order, so "interest already paid
 * this month?" is a single comparison of two ints. */
int date_month_key(Date d)
{
    return d.year * 100 + d.month;
}

/* Writes a month key such as 202612 into buf as "2026-12". */
void date_format_month(int key, char *buf, size_t size)
{
    snprintf(buf, size, "%04d-%02d", key / 100, key % 100);
}

/* Returns today's date from the computer's clock. If the clock cannot be
 * read, returns 1970-01-01, which is earlier than any ledger entry, so the
 * start-up date check stops the program instead of posting wrong dates. */
Date date_today(void)
{
    Date d = { 1970, 1, 1 };
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    if (local != NULL) {
        d.year = local->tm_year + 1900;
        d.month = local->tm_mon + 1;
        d.day = local->tm_mday;
    }
    return d;
}
