/* porter.h - the Porter stemming algorithm (M. F. Porter, "An algorithm for
 * suffix stripping", Program 14(3), 1980) in C11.
 *
 * A stemmer removes suffixes so that related word forms share one key:
 * connect, connected, connecting, connection and connections all become
 * "connect". Stems are not always dictionary words ("ponies" -> "poni").
 */
#ifndef MYCPLUS_PORTER_H
#define MYCPLUS_PORTER_H

#include <stddef.h>

/* PORTER_STANDARD follows Martin Porter's own reference implementation,
 * which departs from the 1980 paper in three small ways: words of one or
 * two letters are left alone, "bli" -> "ble" replaces "abli" -> "able",
 * and "logi" -> "log" is added to step 2. PORTER_PAPER follows the paper
 * exactly. */
enum porter_mode { PORTER_STANDARD = 0, PORTER_PAPER = 1 };

/* Stems `word` in place and returns its new length. The word must be a
 * NUL-terminated lowercase string; letters other than a, e, i, o, u and y
 * are treated as consonants. The stem is never longer than the word. */
size_t porter_stem(char *word, enum porter_mode mode);

/* As porter_stem, calling `trace` after each of the eight steps (1a, 1b,
 * 1c, 2, 3, 4, 5a, 5b) with the step's name and the word so far. */
typedef void (*porter_trace_fn)(const char *step, const char *word, void *ctx);
size_t porter_stem_traced(char *word, enum porter_mode mode,
                          porter_trace_fn trace, void *ctx);

/* m, the "measure" of the paper: the number of vowel-consonant sequences
 * in the first n letters of b, so that b reads [C](VC){m}[V].
 * tree -> 0, trouble -> 1, troubles -> 2. Exposed for teaching and tests. */
unsigned porter_measure(const char *b, size_t n);

#endif
