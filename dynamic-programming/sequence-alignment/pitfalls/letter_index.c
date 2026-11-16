/* letter_index.c - looking up a score with toupper(c) - 'A' in a table
 * that has one entry for each letter from A to Y. Z is a legal protein
 * letter (glutamate or glutamine), and it reads past the end of the table.
 * The entries are BLOSUM62's scores for a letter against itself. */
#include <ctype.h>
#include <stdio.h>

/*                           A  B  C  D  E  F  G  H  I  J  K  L  M */
static const int SELF[25] = {4, 0, 9, 6, 5, 6, 6, 8, 4, 0, 5, 4, 5,
/*                           N  O  P  Q  R  S  T  U  V  W  X  Y */
                             6, 0, 7, 5, 5, 4, 5, 0, 4, 11, 0, 7};

static int self_score(const char *s)
{
    int total = 0;
    for (; *s != '\0'; s++)
        total += SELF[toupper((unsigned char)*s) - 'A'];
    return total;
}

int main(void)
{
    printf("MKTAYIAK  %d\n", self_score("MKTAYIAK"));
    printf("MKTZYIAK  %d\n", self_score("MKTZYIAK"));
    return 0;
}
