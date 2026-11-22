/* An assert whose condition does work. Built with NDEBUG (as release builds
 * usually are), the whole expression disappears, and so does the work. */
#include <assert.h>
#include <stdio.h>

static int items = 3;

static int take_item(void)
{
    items--;
    return items >= 0;
}

int main(void)
{
    assert(take_item()); /* WRONG: the call vanishes when NDEBUG is defined */
#ifdef NDEBUG
    printf("NDEBUG build: %d items left\n", items);
#else
    printf("debug build:  %d items left\n", items);
#endif
    return 0;
}
