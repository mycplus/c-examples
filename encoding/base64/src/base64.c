#include <stdint.h>

#include "base64.h"

static const char ENCODE[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Reverse lookup, built at compile time rather than lazily at run time.
   Indexed by unsigned char, so a byte >= 0x80 cannot produce a negative
   subscript. VALID says whether a character is in the alphabet at all,
   which is what stops an invalid byte quietly decoding to zero. */
static const unsigned char DECODE[256] = {
    ['A']= 0,['B']= 1,['C']= 2,['D']= 3,['E']= 4,['F']= 5,['G']= 6,['H']= 7,
    ['I']= 8,['J']= 9,['K']=10,['L']=11,['M']=12,['N']=13,['O']=14,['P']=15,
    ['Q']=16,['R']=17,['S']=18,['T']=19,['U']=20,['V']=21,['W']=22,['X']=23,
    ['Y']=24,['Z']=25,['a']=26,['b']=27,['c']=28,['d']=29,['e']=30,['f']=31,
    ['g']=32,['h']=33,['i']=34,['j']=35,['k']=36,['l']=37,['m']=38,['n']=39,
    ['o']=40,['p']=41,['q']=42,['r']=43,['s']=44,['t']=45,['u']=46,['v']=47,
    ['w']=48,['x']=49,['y']=50,['z']=51,['0']=52,['1']=53,['2']=54,['3']=55,
    ['4']=56,['5']=57,['6']=58,['7']=59,['8']=60,['9']=61,['+']=62,['/']=63,
};

static const unsigned char VALID[256] = {
    ['A']=1,['B']=1,['C']=1,['D']=1,['E']=1,['F']=1,['G']=1,['H']=1,
    ['I']=1,['J']=1,['K']=1,['L']=1,['M']=1,['N']=1,['O']=1,['P']=1,
    ['Q']=1,['R']=1,['S']=1,['T']=1,['U']=1,['V']=1,['W']=1,['X']=1,
    ['Y']=1,['Z']=1,['a']=1,['b']=1,['c']=1,['d']=1,['e']=1,['f']=1,
    ['g']=1,['h']=1,['i']=1,['j']=1,['k']=1,['l']=1,['m']=1,['n']=1,
    ['o']=1,['p']=1,['q']=1,['r']=1,['s']=1,['t']=1,['u']=1,['v']=1,
    ['w']=1,['x']=1,['y']=1,['z']=1,['0']=1,['1']=1,['2']=1,['3']=1,
    ['4']=1,['5']=1,['6']=1,['7']=1,['8']=1,['9']=1,['+']=1,['/']=1,
};

/* Count groups without computing n + 2, which wraps for n near SIZE_MAX.
   On overflow, return B64_ERROR - which is SIZE_MAX, a size no allocation
   can satisfy, so a caller who ignores the check still fails safely. */
size_t b64_encoded_size(size_t n)
{
    const size_t groups = n / 3 + (n % 3 != 0);
    if (groups > (SIZE_MAX - 1) / 4)
        return B64_ERROR;
    return groups * 4 + 1;
}
size_t b64_decoded_size(size_t n) { return n / 4 * 3; }

size_t b64_encode(const unsigned char *in, size_t len,
                  char *out, size_t out_size)
{
    if (out == NULL || (in == NULL && len > 0))
        return B64_ERROR;
    const size_t need = b64_encoded_size(len);
    if (need == B64_ERROR || out_size < need)
        return B64_ERROR;

    size_t j = 0;
    for (size_t i = 0; i < len; i += 3) {
        const unsigned a = in[i];
        const unsigned b = (i + 1 < len) ? in[i + 1] : 0u;
        const unsigned c = (i + 2 < len) ? in[i + 2] : 0u;
        const unsigned triple = (a << 16) | (b << 8) | c;

        out[j++] = ENCODE[(triple >> 18) & 0x3F];
        out[j++] = ENCODE[(triple >> 12) & 0x3F];
        out[j++] = (i + 1 < len) ? ENCODE[(triple >> 6) & 0x3F] : '=';
        out[j++] = (i + 2 < len) ? ENCODE[triple & 0x3F]        : '=';
    }

    out[j] = '\0';
    return j;
}

size_t b64_decode(const char *in, size_t len,
                  unsigned char *out, size_t out_size)
{
    if (in == NULL || out == NULL)
        return B64_ERROR;
    if (len == 0)
        return 0;                       /* empty input, empty output */
    if (len % 4 != 0)
        return B64_ERROR;

    size_t pad = 0;
    if (in[len - 1] == '=') pad++;
    if (pad == 1 && in[len - 2] == '=') pad++;

    const size_t produced = len / 4 * 3 - pad;
    if (out_size < produced)
        return B64_ERROR;

    size_t j = 0;
    for (size_t i = 0; i < len; i += 4) {
        unsigned quad[4];
        int seen_pad = 0;

        for (int k = 0; k < 4; ++k) {
            const unsigned char ch = (unsigned char)in[i + k];

            if (ch == '=') {
                /* Padding is legal only in the last group, and only in
                   its third or fourth position. */
                if (i + 4 != len || k < 2)
                    return B64_ERROR;
                seen_pad = 1;
                quad[k] = 0;
                continue;
            }
            /* Once padding has started, nothing but padding may follow. */
            if (seen_pad)
                return B64_ERROR;
            if (!VALID[ch])
                return B64_ERROR;
            quad[k] = DECODE[ch];
        }

        /* Reject non-canonical encodings. The bits a padded group does not
           use must be zero - otherwise "Zh==" and "Zg==" would both decode
           to "f", and two different strings would mean the same bytes. */
        if (i + 4 == len) {
            if (pad == 2 && (quad[1] & 0x0F) != 0) return B64_ERROR;
            if (pad == 1 && (quad[2] & 0x03) != 0) return B64_ERROR;
        }

        const unsigned triple = (quad[0] << 18) | (quad[1] << 12)
                              | (quad[2] <<  6) |  quad[3];

        if (j < produced) out[j++] = (unsigned char)((triple >> 16) & 0xFF);
        if (j < produced) out[j++] = (unsigned char)((triple >>  8) & 0xFF);
        if (j < produced) out[j++] = (unsigned char)( triple        & 0xFF);
    }

    return j;
}
