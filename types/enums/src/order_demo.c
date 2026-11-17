/* order_demo: applies status updates to orders, the way an order service
   would. Each line of the input is "ORDER_ID new_status". Every order starts
   as pending; an update that breaks the rules is rejected.

   usage: order_demo EVENTS_FILE */
#include "order.h"

#include <stdio.h>

#define MAX_ORDERS 16

struct order {
    int id;
    order_status status;
};

static struct order *find_or_add(struct order *orders, int *count, int id)
{
    for (int i = 0; i < *count; i++)
        if (orders[i].id == id)
            return &orders[i];
    if (*count == MAX_ORDERS)
        return NULL;
    orders[*count].id = id;
    orders[*count].status = ORDER_PENDING;
    return &orders[(*count)++];
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: order_demo EVENTS_FILE\n");
        return 2;
    }
    FILE *in = fopen(argv[1], "r");
    if (in == NULL) {
        fprintf(stderr, "order_demo: cannot open %s\n", argv[1]);
        return 2;
    }

    struct order orders[MAX_ORDERS];
    int count = 0, rejected = 0;
    char line[128], word[32];
    int id;
    while (fgets(line, sizeof line, in) != NULL) {
        if (line[0] == '#' || sscanf(line, "%d %31s", &id, word) != 2)
            continue;
        order_status next;
        if (!order_status_parse(word, &next)) {
            printf("order %d: \"%s\" is not a status, ignored\n", id, word);
            rejected++;
            continue;
        }
        struct order *o = find_or_add(orders, &count, id);
        if (o == NULL) {
            fprintf(stderr, "order_demo: too many orders\n");
            break;
        }
        if (!order_can_move(o->status, next)) {
            printf("order %d: %s -> %s not allowed\n", id, order_status_name(o->status), order_status_name(next));
            rejected++;
            continue;
        }
        printf("order %d: %s -> %s\n", id, order_status_name(o->status), order_status_name(next));
        o->status = next;
    }
    fclose(in);

    printf("\nFinal status (stored as numbers):\n");
    for (int i = 0; i < count; i++)
        printf("  order %d  %-9s = %d\n", orders[i].id, order_status_name(orders[i].status), (int)orders[i].status);
    printf("%d update%s rejected\n", rejected, rejected == 1 ? "" : "s");
    return 0;
}
