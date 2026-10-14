/* round_trip_only.c - why "decrypt(encrypt(x)) == x" is not a test of a cipher.
 * One bit of one subkey is flipped, as a key-schedule bug would do. The
 * damaged cipher still decrypts everything it encrypts, but it is no longer
 * Serpent: it agrees with none of the published vectors. */
#include "serpent.h"
#include "../tests/vectors.h"

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "tests/vectors/serpent_nessie.txt";
    serpent_vector *v = NULL;
    size_t n = load_vectors(path, &v);
    if (n == 0)
        return EXIT_FAILURE;

    size_t round_trips = 0, vector_matches = 0;
    for (size_t i = 0; i < n; ++i) {
        serpent_key ks;
        serpent_set_key(&ks, v[i].key, v[i].key_len);
        ks.subkey[5][0] ^= 1u;                       /* the "bug" */
        uint8_t out[16], back[16];
        serpent_encrypt(&ks, v[i].plain, out);
        serpent_decrypt(&ks, out, back);
        if (memcmp(back, v[i].plain, 16) == 0)
            ++round_trips;
        if (memcmp(out, v[i].cipher, 16) == 0)
            ++vector_matches;
    }
    printf("round trip succeeds:    %zu of %zu\n", round_trips, n);
    printf("matches the test vector: %zu of %zu\n", vector_matches, n);
    free(v);
    return 0;
}
