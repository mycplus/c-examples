/* Must not compile anywhere: the static assertion documents the on-disk
 * size of a header, and padding makes the struct larger than that. */
#include <assert.h>
#include <stdint.h>

struct header {
    uint32_t magic;
    uint16_t version;
    uint8_t flags; /* the compiler adds one byte of padding after this */
};

static_assert(sizeof(struct header) == 7, "header must be 7 bytes on disk");

int main(void)
{
    return 0;
}
