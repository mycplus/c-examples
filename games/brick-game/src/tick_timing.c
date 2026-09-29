/* tick_timing.c - why the game loop sleeps until a deadline instead of
 * sleeping for a fixed delay. Each tick does WORK_MS of busy work, standing
 * in for drawing on a slow terminal, then waits for the next tick.
 * Usage: tick_timing   (uses term.c's clock; takes about 20 seconds)
 */
#include "term.h"

#include <stdio.h>
#include <stdlib.h>

enum { TICKS = 200, TICK_MS = 50, WORK_MS = 6 };

static void busy_work(uint32_t ms)
{
    uint64_t end = term_now_ms() + ms;
    while (term_now_ms() < end)
        ;
}

int main(void)
{
    uint64_t start = term_now_ms();
    for (int i = 0; i < TICKS; ++i) {           /* delay-based loop */
        busy_work(WORK_MS);
        term_sleep_ms(TICK_MS);
    }
    uint64_t delay_ms = term_now_ms() - start;

    start = term_now_ms();
    uint64_t next = start;
    for (int i = 0; i < TICKS; ++i) {           /* deadline-based loop */
        busy_work(WORK_MS);
        next += TICK_MS;
        uint64_t now = term_now_ms();
        if (next > now)
            term_sleep_ms((uint32_t)(next - now));
    }
    uint64_t deadline_ms = term_now_ms() - start;

    printf("%d ticks of %d ms with %d ms of work per tick (target %d ms):\n",
           TICKS, TICK_MS, WORK_MS, TICKS * TICK_MS);
    printf("  fixed delay:    %llu ms, %.1f ms per tick\n",
           (unsigned long long)delay_ms, (double)delay_ms / TICKS);
    printf("  fixed deadline: %llu ms, %.1f ms per tick\n",
           (unsigned long long)deadline_ms, (double)deadline_ms / TICKS);
    return EXIT_SUCCESS;
}
