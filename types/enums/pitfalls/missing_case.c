/* Pitfall: a switch that misses a constant after the enum grows.

   ORDER_REFUNDED was added to the enum, but delivery_days() was not
   updated. With a default branch the compiler would stay silent; without
   one, GCC and Clang warn with -Wall (-Wswitch). This file is built with
   that warning allowed so the behavior can be shown; the test suite also
   builds it with the warning as an error and expects that build to fail. */
#include <stdio.h>

enum status { PENDING, PAID, SHIPPED, DELIVERED, REFUNDED };

static int delivery_days(enum status s)
{
    switch (s) {
    case PENDING:   return 5;
    case PAID:      return 4;
    case SHIPPED:   return 2;
    case DELIVERED: return 0;
    }
    return -1;          /* reached only for REFUNDED */
}

int main(void)
{
    for (int s = PENDING; s <= REFUNDED; s++)
        printf("status %d: %d days\n", s, delivery_days((enum status)s));
    return 0;
}
