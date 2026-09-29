/* term.h - the platform layer: raw keyboard input, output and a clock. */
#ifndef TERM_H
#define TERM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum { KEY_NONE, KEY_LEFT, KEY_RIGHT, KEY_PAUSE, KEY_QUIT } Key;

bool     term_init(void);      /* raw mode, hidden cursor; false on failure */
void     term_restore(void);   /* safe to call more than once                */
Key      term_key(void);       /* next pending key, or KEY_NONE; never blocks */
void     term_write(const char *s, size_t len);
uint64_t term_now_ms(void);    /* monotonic milliseconds                     */
void     term_sleep_ms(uint32_t ms);

#endif
