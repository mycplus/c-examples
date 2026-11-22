/* Must not compile with -Werror on GCC or Clang: the returned status of a
 * DP_NODISCARD function is ignored, so a parse error would go unnoticed. */
#include "../src/defensive.h"

int main(void)
{
    long port = 8080;
    dp_parse_long("80x", 1, 65535, &port);
    return (int)(port % 256);
}
