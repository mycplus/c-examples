/* porter.c - the Porter stemming algorithm in C11, written from the 1980
 * paper's rules. See porter.h for the interface. */
#include "porter.h"

#include <stdbool.h>
#include <string.h>

typedef struct {
    char *b;               /* the word, NUL-terminated                       */
    size_t k;              /* its current length                             */
    size_t j;              /* length of the stem before the matched suffix   */
    enum porter_mode mode;
} word;

/* A consonant is a letter other than a, e, i, o, u, and other than a y
 * that follows a consonant. So y is a consonant in "yes" and "toy" and a
 * vowel in "sky" and "syzygy". */
static bool is_consonant(const char *b, size_t i)
{
    switch (b[i]) {
    case 'a': case 'e': case 'i': case 'o': case 'u':
        return false;
    case 'y':
        return i == 0 || !is_consonant(b, i - 1);
    default:
        return true;
    }
}

unsigned porter_measure(const char *b, size_t n)
{
    unsigned m = 0;
    size_t i = 0;
    while (i < n && is_consonant(b, i))      /* optional leading consonants */
        ++i;
    while (i < n) {
        while (i < n && !is_consonant(b, i)) /* a vowel sequence ...        */
            ++i;
        if (i == n)
            break;
        while (i < n && is_consonant(b, i))  /* ... closed by consonants    */
            ++i;
        ++m;
    }
    return m;
}

/* *v* in the paper: the first n letters contain a vowel. */
static bool has_vowel(const char *b, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        if (!is_consonant(b, i))
            return true;
    return false;
}

/* *d: the first n letters end with a double consonant, such as -tt or -ss. */
static bool ends_double_consonant(const char *b, size_t n)
{
    return n >= 2 && b[n - 1] == b[n - 2] && is_consonant(b, n - 1);
}

/* *o: the first n letters end consonant-vowel-consonant, and the last
 * consonant is not w, x or y: hop, lov(e), cav(e), but not snow, box, tray. */
static bool ends_cvc(const char *b, size_t n)
{
    if (n < 3 || !is_consonant(b, n - 1) || is_consonant(b, n - 2) || !is_consonant(b, n - 3))
        return false;
    char c = b[n - 1];
    return c != 'w' && c != 'x' && c != 'y';
}

/* If the word ends with `suffix`, sets w->j to the stem's length and
 * returns true. */
static bool ends_with(word *w, const char *suffix)
{
    size_t n = strlen(suffix);
    if (n > w->k || memcmp(w->b + w->k - n, suffix, n) != 0)
        return false;
    w->j = w->k - n;
    return true;
}

/* Replaces everything after the stem (the matched suffix) with `s`. */
static void replace_suffix(word *w, const char *s)
{
    size_t n = strlen(s);
    memcpy(w->b + w->j, s, n);
    w->k = w->j + n;
    w->b[w->k] = '\0';
}

static void truncate_to(word *w, size_t k)
{
    w->k = k;
    w->b[k] = '\0';
}

/* Step 1a: plurals. caresses -> caress, ponies -> poni, cats -> cat. */
static void step1a(word *w)
{
    if (ends_with(w, "sses"))
        truncate_to(w, w->k - 2);
    else if (ends_with(w, "ies"))
        replace_suffix(w, "i");
    else if (ends_with(w, "ss"))
        ;                                   /* caress stays caress */
    else if (ends_with(w, "s"))
        truncate_to(w, w->k - 1);
}

/* Step 1b: -ed and -ing. agreed -> agree, plastered -> plaster,
 * hopping -> hop, filing -> file. */
static void step1b(word *w)
{
    if (ends_with(w, "eed")) {
        if (porter_measure(w->b, w->j) > 0)
            truncate_to(w, w->k - 1);       /* agreed -> agree; feed stays */
        return;
    }
    if (!((ends_with(w, "ed") || ends_with(w, "ing")) && has_vowel(w->b, w->j)))
        return;                             /* sing and bed are left alone */
    truncate_to(w, w->j);

    /* Tidy up the stem the suffix left behind. */
    if (ends_with(w, "at") || ends_with(w, "bl") || ends_with(w, "iz")) {
        w->b[w->k] = 'e';                   /* conflat -> conflate */
        truncate_to(w, w->k + 1);
    } else if (ends_double_consonant(w->b, w->k)) {
        char c = w->b[w->k - 1];
        if (c != 'l' && c != 's' && c != 'z')
            truncate_to(w, w->k - 1);       /* hopp -> hop; fall stays */
    } else if (porter_measure(w->b, w->k) == 1 && ends_cvc(w->b, w->k)) {
        w->b[w->k] = 'e';                   /* fil -> file */
        truncate_to(w, w->k + 1);
    }
}

/* Step 1c: a final y becomes i when the stem has a vowel. happy -> happi. */
static void step1c(word *w)
{
    if (ends_with(w, "y") && has_vowel(w->b, w->j))
        w->b[w->k - 1] = 'i';
}

typedef struct {
    const char *suffix, *replacement;
    signed char only;    /* -1 every mode, else only this porter_mode */
} rule;

/* Steps 2 to 4 apply the rule with the LONGEST matching suffix, and only
 * that one: if its condition fails, the step does nothing. */
static void apply_longest(word *w, const rule *rules, size_t count, unsigned min_m)
{
    const rule *best = NULL;
    size_t best_len = 0;
    for (size_t i = 0; i < count; ++i) {
        size_t n = strlen(rules[i].suffix);
        if ((rules[i].only < 0 || rules[i].only == (int)w->mode) && n > best_len &&
            ends_with(w, rules[i].suffix)) {
            best = &rules[i];
            best_len = n;
        }
    }
    if (best == NULL)
        return;
    w->j = w->k - best_len;
    if (strcmp(best->suffix, "ion") == 0 && /* step 4: -ion needs s or t */
        (w->j == 0 || (w->b[w->j - 1] != 's' && w->b[w->j - 1] != 't')))
        return;
    if (porter_measure(w->b, w->j) > min_m)
        replace_suffix(w, best->replacement);
}

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

/* Step 2: double suffixes to single ones, if m > 0. */
static const rule step2_rules[] = {
    {"ational", "ate", -1}, {"tional", "tion", -1}, {"enci", "ence", -1},
    {"anci", "ance", -1},   {"izer", "ize", -1},    {"abli", "able", PORTER_PAPER},
    {"bli", "ble", PORTER_STANDARD},                {"alli", "al", -1},
    {"entli", "ent", -1},   {"eli", "e", -1},       {"ousli", "ous", -1},
    {"ization", "ize", -1}, {"ation", "ate", -1},   {"ator", "ate", -1},
    {"alism", "al", -1},    {"iveness", "ive", -1}, {"fulness", "ful", -1},
    {"ousness", "ous", -1}, {"aliti", "al", -1},    {"iviti", "ive", -1},
    {"biliti", "ble", -1},  {"logi", "log", PORTER_STANDARD},
};

/* Step 3: -ic-, -full, -ness and friends, if m > 0. */
static const rule step3_rules[] = {
    {"icate", "ic", -1}, {"ative", "", -1}, {"alize", "al", -1}, {"iciti", "ic", -1},
    {"ical", "ic", -1},  {"ful", "", -1},   {"ness", "", -1},
};

/* Step 4: remove the suffix outright, if m > 1. */
static const rule step4_rules[] = {
    {"al", "", -1},   {"ance", "", -1}, {"ence", "", -1}, {"er", "", -1},
    {"ic", "", -1},   {"able", "", -1}, {"ible", "", -1}, {"ant", "", -1},
    {"ement", "", -1}, {"ment", "", -1}, {"ent", "", -1}, {"ion", "", -1},
    {"ou", "", -1},   {"ism", "", -1},  {"ate", "", -1},  {"iti", "", -1},
    {"ous", "", -1},  {"ive", "", -1},  {"ize", "", -1},
};

/* Step 5a: a final e goes if m > 1, or if m = 1 and the stem is not *o.
 * probate -> probat, cease -> ceas, but rate stays rate. */
static void step5a(word *w)
{
    if (!ends_with(w, "e"))
        return;
    unsigned m = porter_measure(w->b, w->j);
    if (m > 1 || (m == 1 && !ends_cvc(w->b, w->j)))
        truncate_to(w, w->j);
}

/* Step 5b: -ll becomes -l if m > 1. controll -> control, roll stays. */
static void step5b(word *w)
{
    if (w->k >= 2 && w->b[w->k - 1] == 'l' && ends_double_consonant(w->b, w->k) &&
        porter_measure(w->b, w->k) > 1)
        truncate_to(w, w->k - 1);
}

size_t porter_stem_traced(char *b, enum porter_mode mode, porter_trace_fn trace, void *ctx)
{
    word w = {b, strlen(b), 0, mode};
    if (mode == PORTER_STANDARD && w.k <= 2)
        return w.k;                         /* "as", "is" and "us" stay whole */

    static const char *const names[] = {"1a", "1b", "1c", "2", "3", "4", "5a", "5b"};
    for (int step = 0; step < 8; ++step) {
        switch (step) {
        case 0: step1a(&w); break;
        case 1: step1b(&w); break;
        case 2: step1c(&w); break;
        case 3: apply_longest(&w, step2_rules, COUNT(step2_rules), 0); break;
        case 4: apply_longest(&w, step3_rules, COUNT(step3_rules), 0); break;
        case 5: apply_longest(&w, step4_rules, COUNT(step4_rules), 1); break;
        case 6: step5a(&w); break;
        default: step5b(&w); break;
        }
        if (trace != NULL)
            trace(names[step], w.b, ctx);
    }
    return w.k;
}

size_t porter_stem(char *word, enum porter_mode mode)
{
    return porter_stem_traced(word, mode, NULL, NULL);
}
