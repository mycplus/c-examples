#ifndef ORDER_H
#define ORDER_H

#include <stdbool.h>

/* The stages of an online order. The numbers are written to the database,
   so each one is fixed explicitly and never reused: adding a stage means
   adding a new number at the end, not renumbering. Zero is left out on
   purpose, so a status that was never set (a zeroed struct) is invalid. */
typedef enum order_status {
    ORDER_PENDING   = 1,
    ORDER_PAID      = 2,
    ORDER_SHIPPED   = 3,
    ORDER_DELIVERED = 4,
    ORDER_CANCELLED = 5,
} order_status;

/* True if v is one of the values above. Check this before converting a
   number from a file, the network or the user into an order_status. */
bool order_status_valid(int v);

/* "pending", "paid", ...; "invalid" for any other value. */
const char *order_status_name(order_status s);

/* Parses a name such as "shipped". Returns false for anything else. */
bool order_status_parse(const char *text, order_status *out);

/* The business rules: which stage may follow which. */
bool order_can_move(order_status from, order_status to);

#endif
