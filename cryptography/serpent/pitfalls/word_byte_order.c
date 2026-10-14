/* word_byte_order.c - reading each 32-bit word big-endian instead of
 * little-endian, as a cast through ntohl() or a big-endian machine would.
 * Simulated by swapping the bytes of every word of the key, plaintext and
 * ciphertext around a correct implementation. */
#include "serpent.h"
#include "../tests/vectors.h"

static void swap_words(uint8_t *p, size_t n)
{
    for (size_t i = 0; i + 4 <= n; i += 4) {
        uint8_t t0 = p[i], t1 = p[i + 1];
        p[i] = p[i + 3];
        p[i + 1] = p[i + 2];
        p[i + 2] = t1;
        p[i + 3] = t0;
    }
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "tests/vectors/serpent_nessie.txt";
    serpent_vector *v = NULL;
    size_t n = load_vectors(path, &v);
    if (n == 0)
        return EXIT_FAILURE;

    size_t matches = 0;
    for (size_t i = 0; i < n; ++i) {
        uint8_t key[32], block[16];
        memcpy(key, v[i].key, v[i].key_len);
        memcpy(block, v[i].plain, 16);
        swap_words(key, v[i].key_len);
        swap_words(block, 16);
        serpent_key ks;
        serpent_set_key(&ks, key, v[i].key_len);
        serpent_encrypt(&ks, block, block);           /* in == out is allowed */
        swap_words(block, 16);
        if (memcmp(block, v[i].cipher, 16) == 0)
            ++matches;
    }
    printf("big-endian words match the test vectors: %zu of %zu\n", matches, n);
    free(v);
    return 0;
}
