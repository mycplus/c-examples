/* lcs.h - the longest common subsequence (LCS) of two strings, and a
 * line-by-line diff built on it, in C11.
 *
 * A subsequence keeps characters in order but may skip some: "ACE" is a
 * subsequence of "ABCDE". The LCS of two strings is a longest string that
 * is a subsequence of both. It is not always unique.
 */
#ifndef MYCPLUS_LCS_H
#define MYCPLUS_LCS_H

#include <stddef.h>
#include <stdio.h>

/* Length of the LCS of a and b, using O(min(n, m)) memory.
 * Returns SIZE_MAX if memory runs out. */
size_t lcs_length(const char *a, const char *b);

/* One LCS of a and b, as a new string the caller must free(). The result
 * is always allocated, even when it is empty. Returns NULL if memory runs
 * out or the table would be too large. Uses O(n * m) memory. */
char *lcs_string(const char *a, const char *b);

/* Prints the (n+1) x (m+1) table of LCS lengths for two short strings. */
void lcs_print_table(const char *a, const char *b, FILE *out);

/* Compares two arrays of lines and prints a diff to out: "  " before a line
 * in both, "- " before a line only in a, "+ " before a line only in b.
 * Returns the number of "-" and "+" lines (na + nb - 2 * LCS), or -1 if
 * memory runs out. */
long lcs_diff(const char *const *a, size_t na, const char *const *b, size_t nb, FILE *out);

#endif
