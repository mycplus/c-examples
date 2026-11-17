/* Pitfall: trusting a number to be a valid enum value.

   C lets any int be stored in an enum variable. Statuses read back from a
   file arrive as plain numbers, and one of them (7) is not an order status,
   because another program wrote it. Using it to index a table of names
   reads past the end of the table.

   usage: pitfall_out_of_range checked     (validates first: safe)
          pitfall_out_of_range unchecked   (indexes directly: undefined behavior) */
#include "order.h"

#include <stdio.h>
#include <string.h>

static const char *const names[] = {
    [ORDER_PENDING] = "pending",
    [ORDER_PAID] = "paid",
    [ORDER_SHIPPED] = "shipped",
    [ORDER_DELIVERED] = "delivered",
    [ORDER_CANCELLED] = "cancelled",
};

int main(int argc, char **argv)
{
    int checked = argc > 1 && strcmp(argv[1], "checked") == 0;
    const int from_file[] = {2, 4, 7};

    for (size_t i = 0; i < sizeof from_file / sizeof from_file[0]; i++) {
        int v = from_file[i];
        if (checked && !order_status_valid(v)) {
            printf("record %zu: status %d is not valid, skipped\n", i, v);
            continue;
        }
        order_status s = (order_status)v;           /* C accepts any value */
        printf("record %zu: status %d = %s\n", i, v, names[s]);
    }
    return 0;
}
