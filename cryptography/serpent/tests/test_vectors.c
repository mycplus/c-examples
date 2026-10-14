/* test_vectors.c - every NESSIE single-block vector, in both directions,
 * plus key-length validation. */
#include "serpent.h"
#include "vectors.h"

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "tests/vectors/serpent_nessie.txt";
    serpent_vector *v = NULL;
    size_t n = load_vectors(path, &v);
    if (n == 0)
        return EXIT_FAILURE;

    size_t per_size[33] = { 0 }, failures = 0;
    for (size_t i = 0; i < n; ++i) {
        serpent_key ks;
        uint8_t out[16], back[16];
        if (serpent_set_key(&ks, v[i].key, v[i].key_len) != 0) {
            ++failures;
            continue;
        }
        serpent_encrypt(&ks, v[i].plain, out);
        serpent_decrypt(&ks, v[i].cipher, back);
        if (memcmp(out, v[i].cipher, 16) != 0 || memcmp(back, v[i].plain, 16) != 0)
            ++failures;
        else
            ++per_size[v[i].key_len];
    }
    printf("%zu vectors: %zu with 128-bit keys, %zu with 192-bit, %zu with 256-bit; %zu failed\n",
           n, per_size[16], per_size[24], per_size[32], failures);

    /* Invalid key lengths are refused. */
    serpent_key ks;
    uint8_t big[33] = { 0 };
    int bad_lengths = serpent_set_key(&ks, big, 0) == -1 && serpent_set_key(&ks, big, 33) == -1;
    /* A 1-byte key is legal: it is padded like any short key. */
    int one_byte = serpent_set_key(&ks, big, 1) == 0;
    printf("rejects 0- and 33-byte keys: %s; accepts a 1-byte key: %s\n",
           bad_lengths ? "yes" : "no", one_byte ? "yes" : "no");

    free(v);
    return failures == 0 && bad_lengths && one_byte ? EXIT_SUCCESS : EXIT_FAILURE;
}
