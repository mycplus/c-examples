/* screen.h - turn a new text frame into the terminal output that draws it. */
#ifndef SCREEN_H
#define SCREEN_H

#include <stddef.h>

/* Writes into out the escape sequences that change the terminal from frame
 * prev to frame cur. Both are NUL-terminated text with '\n' line breaks.
 * The two frames must have the same line lengths, as frames of one game do.
 * Pass prev == NULL to clear the screen and draw cur in full.
 * Returns the length needed, excluding the NUL; if >= cap, out is truncated. */
size_t screen_diff(const char *prev, const char *cur, char *out, size_t cap);

#endif
