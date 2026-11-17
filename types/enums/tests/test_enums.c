/* Tests for the order-status library. */
#include "order.h"

#include <stdio.h>
#include <string.h>

static int checks, failures;
#define CHECK(cond)                                                       \
    do {                                                                  \
        checks++;                                                         \
        if (!(cond)) {                                                    \
            failures++;                                                   \
            fprintf(stderr, "FAILED line %d: %s\n", __LINE__, #cond);     \
        }                                                                 \
    } while (0)

int main(void)
{
    /* Valid values are exactly 1 to 5; 0 (a zeroed struct) is not one. */
    for (int v = -3; v <= 9; v++)
        CHECK(order_status_valid(v) == (v >= 1 && v <= 5));

    /* Every status has its own name, and parsing the name gives it back. */
    for (int v = ORDER_PENDING; v <= ORDER_CANCELLED; v++) {
        order_status back = ORDER_PENDING;
        const char *name = order_status_name((order_status)v);
        CHECK(strcmp(name, "invalid") != 0);
        CHECK(order_status_parse(name, &back) && back == (order_status)v);
        for (int w = ORDER_PENDING; w < v; w++)
            CHECK(strcmp(name, order_status_name((order_status)w)) != 0);
    }
    CHECK(strcmp(order_status_name((order_status)0), "invalid") == 0);
    CHECK(strcmp(order_status_name((order_status)6), "invalid") == 0);

    /* Parsing is exact: no other spelling or unknown word is accepted. */
    order_status s = ORDER_PAID;
    CHECK(!order_status_parse("Paid", &s));
    CHECK(!order_status_parse("paid ", &s));
    CHECK(!order_status_parse("", &s));
    CHECK(!order_status_parse("refunded", &s));
    CHECK(!order_status_parse("invalid", &s));
    CHECK(s == ORDER_PAID);                 /* untouched on failure */

    /* The complete table of allowed moves: row = from, column = to. */
    static const int allowed[6][6] = {
        [ORDER_PENDING] = {[ORDER_PAID] = 1, [ORDER_CANCELLED] = 1},
        [ORDER_PAID] = {[ORDER_SHIPPED] = 1, [ORDER_CANCELLED] = 1},
        [ORDER_SHIPPED] = {[ORDER_DELIVERED] = 1},
    };
    for (int from = ORDER_PENDING; from <= ORDER_CANCELLED; from++)
        for (int to = ORDER_PENDING; to <= ORDER_CANCELLED; to++)
            CHECK(order_can_move((order_status)from, (order_status)to) == (allowed[from][to] == 1));
    CHECK(!order_can_move((order_status)0, ORDER_PAID));
    CHECK(!order_can_move((order_status)9, ORDER_PAID));

    if (failures) {
        printf("%d of %d checks failed\n", failures, checks);
        return 1;
    }
    printf("all %d checks passed\n", checks);
    return 0;
}
