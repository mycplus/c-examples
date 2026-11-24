/* money.h - amounts of money as whole cents.
 *
 * A double cannot hold 0.10 exactly (it stores the nearest binary
 * fraction), so 0.1 + 0.2 != 0.3 and errors creep into every balance.
 * Every amount in this program is therefore a count of cents in a
 * long long: 1,234.50 is stored as 123450. Only the screen shows a point. */
#ifndef MONEY_H
#define MONEY_H

#include <stddef.h>

typedef long long Cents;                 /* money in whole cents, never float/double */

#define MAX_AMOUNT        100000000LL    /* 1,000,000.00: largest single transaction */
#define MAX_BALANCE       99999999999LL  /* 999,999,999.99: largest balance */
#define MONEY_TEXT_SIZE   32             /* enough for "-999,999,999.99" with room to spare */

int   money_parse(const char *text, Cents *out);          /* 1 if text is a valid amount */
void  money_format(Cents value, char *buf, size_t size);  /* "1,234.50", "-399.50", "0.05" */
Cents interest_for_month(Cents balance, int rate_bp);     /* half up; 0 if balance <= 0 */

#endif
