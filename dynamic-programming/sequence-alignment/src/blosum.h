/* blosum.h - amino-acid scoring tables for use with the Scoring struct in
 * align.h: Scoring s = { BLOSUM_LETTERS, BLOSUM62, 11, 1 }; */
#ifndef BLOSUM_H
#define BLOSUM_H

extern const char BLOSUM_LETTERS[];     /* 24 letters */
extern const int BLOSUM62[24 * 24];
extern const int BLOSUM50[24 * 24];

#endif
