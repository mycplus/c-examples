/* Pitfall: inserting a new constant in the middle of an enum whose values
   are stored outside the program.

   Version 1 of a shop saved order statuses to its database as numbers.
   Version 2 added ON_HOLD after PAID, without explicit values, so every
   later constant moved up by one. The old records now read back wrong. */
#include <stdio.h>
#include <string.h>

enum status_v1 { V1_PENDING, V1_PAID, V1_SHIPPED, V1_DELIVERED };
static const char *const names_v1[] = {"pending", "paid", "shipped", "delivered"};

enum status_v2 { V2_PENDING, V2_PAID, V2_ON_HOLD, V2_SHIPPED, V2_DELIVERED };
static const char *const names_v2[] = {"pending", "paid", "on hold", "shipped", "delivered"};

int main(void)
{
    const int saved_by_v1[] = {V1_PAID, V1_SHIPPED, V1_DELIVERED};
    printf("%-6s %-10s %-10s\n", "value", "saved as", "read as");
    for (int i = 0; i < 3; i++) {
        int v = saved_by_v1[i];
        printf("%-6d %-10s %-10s%s\n", v, names_v1[v], names_v2[v],
               strcmp(names_v1[v], names_v2[v]) != 0 ? "  <- wrong" : "");
    }
    return 0;
}
