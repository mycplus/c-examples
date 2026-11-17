/* flags: an enum of single bits, combined into a set of permissions,
   as in a file-sharing app. Each constant is one bit, so any combination
   fits in one unsigned int. */
#include <stdio.h>

enum permission {
    PERM_READ   = 1u << 0,
    PERM_WRITE  = 1u << 1,
    PERM_SHARE  = 1u << 2,
    PERM_DELETE = 1u << 3,
};

/* A combination such as READ | WRITE is not one of the enum's constants,
   so the set is kept in an unsigned int rather than an enum permission. */
typedef unsigned int permissions;

static void show(const char *who, permissions p)
{
    printf("%-8s 0x%X:", who, p);
    if (p == 0)
        printf(" none");
    if (p & PERM_READ)
        printf(" read");
    if (p & PERM_WRITE)
        printf(" write");
    if (p & PERM_SHARE)
        printf(" share");
    if (p & PERM_DELETE)
        printf(" delete");
    putchar('\n');
}

int main(void)
{
    permissions viewer = PERM_READ;
    permissions editor = PERM_READ | PERM_WRITE;
    permissions owner = PERM_READ | PERM_WRITE | PERM_SHARE | PERM_DELETE;
    show("viewer", viewer);
    show("editor", editor);
    show("owner", owner);

    editor |= PERM_SHARE;                   /* grant */
    show("editor+", editor);
    owner &= ~(permissions)PERM_DELETE;     /* revoke */
    show("owner-", owner);

    /* Test for all of several bits at once: compare with the mask. */
    permissions needed = PERM_READ | PERM_SHARE;
    printf("viewer can read and share? %s\n", (viewer & needed) == needed ? "yes" : "no");
    printf("editor can read and share? %s\n", (editor & needed) == needed ? "yes" : "no");
    return 0;
}
