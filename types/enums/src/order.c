#include "order.h"

#include <string.h>

bool order_status_valid(int v)
{
    return v >= ORDER_PENDING && v <= ORDER_CANCELLED;
}

/* A switch with no default: if a new status is added and not handled
   here, GCC and Clang warn (-Wswitch, part of -Wall). */
const char *order_status_name(order_status s)
{
    switch (s) {
    case ORDER_PENDING:   return "pending";
    case ORDER_PAID:      return "paid";
    case ORDER_SHIPPED:   return "shipped";
    case ORDER_DELIVERED: return "delivered";
    case ORDER_CANCELLED: return "cancelled";
    }
    return "invalid";
}

bool order_status_parse(const char *text, order_status *out)
{
    for (int v = ORDER_PENDING; v <= ORDER_CANCELLED; v++) {
        if (strcmp(text, order_status_name((order_status)v)) == 0) {
            *out = (order_status)v;
            return true;
        }
    }
    return false;
}

bool order_can_move(order_status from, order_status to)
{
    switch (from) {
    case ORDER_PENDING:   return to == ORDER_PAID || to == ORDER_CANCELLED;
    case ORDER_PAID:      return to == ORDER_SHIPPED || to == ORDER_CANCELLED;
    case ORDER_SHIPPED:   return to == ORDER_DELIVERED;
    case ORDER_DELIVERED: return false;
    case ORDER_CANCELLED: return false;
    }
    return false;
}
