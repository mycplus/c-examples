/* atoi cannot report errors: "abc" and "0" both give 0, and "12abc" gives 12.
 * (A number too large for int is undefined behavior in atoi, so it is not
 * tried here.) dp_parse_long, built on strtol, tells the cases apart. */
#include "../src/defensive.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    const char *inputs[] = {"42", "0", "abc", "12abc", "", " 7", "-5"};
    printf("%-8s %-6s %s\n", "input", "atoi", "dp_parse_long (range 0 to 100)");
    for (size_t i = 0; i < sizeof inputs / sizeof inputs[0]; i++) {
        long value = 0;
        const dp_status st = dp_parse_long(inputs[i], 0, 100, &value);
        char quoted[16];
        snprintf(quoted, sizeof quoted, "\"%s\"", inputs[i]);
        if (st == DP_OK) {
            printf("%-8s %-6d %ld\n", quoted, atoi(inputs[i]), value);
        } else {
            printf("%-8s %-6d error: %s\n", quoted, atoi(inputs[i]), dp_strerror(st));
        }
    }
    return 0;
}
