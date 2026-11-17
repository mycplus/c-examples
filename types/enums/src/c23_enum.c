/* c23_enum: C23 lets an enum name its underlying type. Built only by
   compilers that support it (GCC 13+, Clang 18+ with -std=c2x or -std=c23). */
#include <stdio.h>

enum status_byte : unsigned char { S_PENDING = 1, S_PAID, S_SHIPPED, S_DELIVERED, S_CANCELLED };
enum status_plain { P_PENDING = 1, P_PAID, P_SHIPPED, P_DELIVERED, P_CANCELLED };

/* Constants may now be wider than int when the type says so. */
enum file_size : long long { MAX_UPLOAD = 5LL * 1024 * 1024 * 1024 };   /* 5 GiB */

struct shipment_small { enum status_byte status; unsigned char warehouse; unsigned short weight_kg; };
struct shipment_plain { enum status_plain status; unsigned char warehouse; unsigned short weight_kg; };

int main(void)
{
    printf("sizeof(enum status_byte)  = %zu\n", sizeof(enum status_byte));
    printf("sizeof(enum status_plain) = %zu\n", sizeof(enum status_plain));
    printf("struct with byte enum:  %zu bytes\n", sizeof(struct shipment_small));
    printf("struct with plain enum: %zu bytes\n", sizeof(struct shipment_plain));
    printf("1,000,000 shipments: %zu vs %zu bytes\n",
           1000000 * sizeof(struct shipment_small), 1000000 * sizeof(struct shipment_plain));
    printf("MAX_UPLOAD = %lld\n", (long long)MAX_UPLOAD);
    return 0;
}
