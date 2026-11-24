/* money.c - reading, printing and computing amounts of money in whole cents.
 * No float or double appears anywhere in this program. */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "money.h"

#define MAX_WHOLE_DIGITS 7   /* 1000000 is the largest whole part an amount can have */

/* Reads an amount typed by a person, such as "250", "250.7" or " 250.75 ",
 * into *out as cents. Rules: optional spaces or tabs before and after; 1 to
 * 7 digits; then optionally a point followed by exactly 1 or 2 digits.
 * Nothing else is allowed: no sign, no thousands commas, no exponent.
 * The value must be from 0.01 to 1,000,000.00 (MAX_AMOUNT).
 * Returns 1 and sets *out if the text is valid, otherwise returns 0 and
 * leaves *out unchanged.
 *
 * Why not strtod()? It returns a double, so "0.29" would come back as
 * 0.28999999999999998 and a careless (long long)(x * 100) gives 28 cents.
 * strtod() also accepts "1e3", "nan", "inf" and hexadecimal. Building the
 * value digit by digit avoids every one of those surprises.
 *
 * Overflow is impossible: at most 7 + 2 digits are ever accumulated, so
 * the value stays below 10^9, far inside a long long. */
int money_parse(const char *text, Cents *out)
{
    const char *p = text;
    Cents value = 0;
    int digits = 0;

    while (*p == ' ' || *p == '\t')
        p++;
    while (isdigit((unsigned char)*p)) {
        if (++digits > MAX_WHOLE_DIGITS)
            return 0;                       /* too many digits: stop before any overflow */
        value = value * 10 + (*p - '0');
        p++;
    }
    if (digits == 0)
        return 0;                           /* ".5", "-5", "+5", "nan" and "" end up here */
    value *= 100;                           /* whole units to cents */
    if (*p == '.') {
        p++;
        if (!isdigit((unsigned char)*p))
            return 0;                       /* "250." */
        value += (*p - '0') * 10;           /* tenths */
        p++;
        if (isdigit((unsigned char)*p)) {
            value += *p - '0';              /* hundredths */
            p++;
        }
        if (isdigit((unsigned char)*p))
            return 0;                       /* "12.505": a third decimal */
    }
    while (*p == ' ' || *p == '\t')
        p++;
    if (*p != '\0')
        return 0;                           /* "1,000", "1e3", "12abc" */
    if (value < 1 || value > MAX_AMOUNT)
        return 0;                           /* "0", "0.00", "1000000.01" */
    *out = value;
    return 1;
}

/* Writes value as people read money into buf: a minus sign if negative,
 * the whole part with a comma every three digits, a point and always two
 * decimals. 0 -> "0.00", 5 -> "0.05", 123456789 -> "1,234,567.89",
 * -39950 -> "-399.50". buf should hold MONEY_TEXT_SIZE characters.
 * No amount here can be LLONG_MIN, so -value never overflows. */
void money_format(Cents value, char *buf, size_t size)
{
    char reversed[MONEY_TEXT_SIZE];
    size_t n = 0;
    int negative = value < 0;
    Cents v = negative ? -value : value;
    Cents whole = v / 100;
    int cents = (int)(v % 100);
    int group = 0;

    /* Build the whole part backwards, digit by digit, adding a comma
     * before every fourth digit counted from the right. */
    do {
        if (group == 3) {
            reversed[n++] = ',';
            group = 0;
        }
        reversed[n++] = (char)('0' + (int)(whole % 10));
        whole /= 10;
        group++;
    } while (whole > 0 && n < sizeof reversed - 2);

    char text[MONEY_TEXT_SIZE];
    size_t len = 0;
    if (negative)
        text[len++] = '-';
    while (n > 0)
        text[len++] = reversed[--n];
    text[len] = '\0';
    snprintf(buf, size, "%s.%02d", text, cents);
}

/* Returns one month's interest on balance at rate_bp basis points a year
 * (300 = 3.00%), rounded half up to the cent, using integers only.
 *
 * A year's interest is balance * rate_bp / 10000, and a month is a twelfth
 * of that, so the divisor is 12 * 10000 = 120000. Integer division always
 * rounds down; adding half the divisor (60000) first turns that into
 * "round half up": 4.626875 becomes 4.63 and exactly 0.005 becomes 0.01.
 *
 * The product cannot overflow: MAX_BALANCE * 300 is about 3 * 10^13, and
 * a long long holds up to about 9.2 * 10^18.
 * Returns 0 when the balance is zero or negative: no interest is charged
 * or paid on a debt by this function. */
Cents interest_for_month(Cents balance, int rate_bp)
{
    if (balance <= 0)
        return 0;
    return (balance * rate_bp + 60000) / 120000;
}
