/*
 * address_book.c - contacts kept in a CSV file that Excel, LibreOffice or
 * any text editor can open.
 *
 * Contacts are loaded at start-up, listed by name or by email, added,
 * searched and deleted, and saved when you choose Save or answer yes when
 * quitting. Names may contain commas and quotes: the CSV file stores them
 * correctly.
 *
 * Build with GCC or Clang:
 *     gcc -std=c11 -Wall -Wextra -o address_book address_book.c
 * Build with Visual Studio (Developer Command Prompt):
 *     cl /W4 address_book.c
 * Run:
 *     ./address_book                    uses contacts.csv
 *     ./address_book --file work.csv    a second address book
 */
#define _CRT_SECURE_NO_WARNINGS     /* fopen, strerror, remove, rename on MSVC */
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE    128            /* read_line buffer (read_text uses it too) */
#define NAME_SIZE    41             /* 40 characters + '\0' */
#define PHONE_SIZE   21
#define EMAIL_SIZE   51
#define SEARCH_SIZE  31
#define FILE_LINE    256            /* one CSV line: 3 quoted fields fit */
#define PATH_SIZE    260
#define MAX_CONTACTS 100
#define DEFAULT_FILE "contacts.csv"
#define CSV_HEADER   "name,phone,email"

/* One contact. */
typedef struct {
    char name[NAME_SIZE];
    char phone[PHONE_SIZE];
    char email[EMAIL_SIZE];
} Contact;

/* The whole address book: an array of structs kept in name order. */
typedef struct {
    Contact contacts[MAX_CONTACTS];   /* always sorted by name */
    int     count;
    bool    changed;                  /* unsaved changes? */
} AddressBook;

static int  read_line(char *buf, size_t size);
static int  read_int(const char *prompt, int lo, int hi, int *value);
static int  read_text(const char *prompt, char *out, size_t out_size);
static int  compare_ignore_case(const char *a, const char *b);
static int  compare_by_name(const void *a, const void *b);
static int  compare_by_email(const void *a, const void *b);
static int  find_by_name(const AddressBook *book, const char *name);
static int  valid_phone(const char *phone);
static int  valid_email(const char *email);
static void write_csv_field(FILE *f, const char *field);
static int  parse_csv_line(const char *line, char fields[3][EMAIL_SIZE]);
static int  load_book(AddressBook *book, const char *path);
static int  save_book(AddressBook *book, const char *path);
static void print_table(const AddressBook *book, const Contact list[], int count);
static void list_by_email(const AddressBook *book);
static int  add_contact(AddressBook *book);
static int  contains_ignore_case(const char *text, const char *word);
static int  search_contacts(const AddressBook *book);
static int  delete_contact(AddressBook *book);
static int  ask_yes_no(const char *prompt);

int main(int argc, char *argv[])
{
    static AddressBook book;                    /* static: about 11 KB, kept off the stack */
    const char *path = DEFAULT_FILE;
    int choice = 0, ok = 1;

    if (argc == 3 && strcmp(argv[1], "--file") == 0 && strlen(argv[2]) < PATH_SIZE - 4) {
        path = argv[2];
    } else if (argc != 1) {
        fprintf(stderr, "Usage: address_book [--file NAME]\n");
        return 1;
    }

    printf("Address Book\n============\n");
    if (!load_book(&book, path))
        return 1;

    /* End of input stops the program without saving: unsaved changes are
     * lost, as when an editor is closed without saving. */
    while (ok) {
        printf("\n1. List contacts by name\n2. List contacts by email\n3. Add a contact\n"
               "4. Search\n5. Delete a contact\n6. Save\n0. Quit\n");
        if (!read_int("Choose an option (0-6): ", 0, 6, &choice))
            break;
        switch (choice) {
        case 1:
            printf("\n");
            print_table(&book, book.contacts, book.count);
            break;
        case 2: list_by_email(&book); break;
        case 3: ok = add_contact(&book); break;
        case 4: ok = search_contacts(&book); break;
        case 5: ok = delete_contact(&book); break;
        case 6: save_book(&book, path); break;
        default:                                /* 0: quit */
            if (book.changed) {
                int answer = ask_yes_no("Save your changes? (y/n): ");
                if (answer < 0)
                    ok = 0;
                else if (answer == 0)
                    printf("Changes discarded.\n");
                else if (!save_book(&book, path))
                    break;                      /* the save failed: back to the menu */
            }
            if (ok) {
                printf("\nGoodbye!\n");
                return 0;
            }
            break;
        }
    }
    printf("\n\nGoodbye!\n");
    return 0;
}

/* Reads one line into buf and removes the newline. If the line is too long,
 * the rest is thrown away so it cannot become the next answer, and buf is
 * emptied so the caller rejects it instead of using half a number.
 * Returns 1 on success, 0 at end of input. */
static int read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL)
        return 0;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else if (len == size - 1) {               /* buffer full: is there more? */
        int c, extra = 0;
        while ((c = getchar()) != '\n' && c != EOF)
            extra = 1;
        if (extra)
            buf[0] = '\0';
    }
    return 1;
}

/* Prints prompt and reads a whole number from lo to hi into *value.
 * Anything else (letters, an empty line, 12abc, out of range) prints a
 * message and asks again. Returns 1 on success, 0 at end of input. */
static int read_int(const char *prompt, int lo, int hi, int *value)
{
    char buf[LINE_SIZE];
    char *end;

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;
        errno = 0;
        long n = strtol(buf, &end, 10);
        while (*end == ' ' || *end == '\t')
            end++;                              /* allow trailing spaces */
        if (end != buf && *end == '\0' && errno == 0 && n >= lo && n <= hi) {
            *value = (int)n;
            return 1;
        }
        printf("Please enter a whole number from %d to %d.\n", lo, hi);
    }
}

/* Prints prompt and reads a line of text, without spaces or tabs at either
 * end, into out. It must be 1 to out_size - 1 characters; anything else
 * (an empty line, a name that is too long) asks again.
 * Returns 1 on success, 0 at end of input. */
static int read_text(const char *prompt, char *out, size_t out_size)
{
    char buf[LINE_SIZE];

    for (;;) {
        char *p = buf, *end;

        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;
        while (*p == ' ' || *p == '\t')
            p++;
        end = p + strlen(p);
        while (end > p && (end[-1] == ' ' || end[-1] == '\t'))
            end--;
        size_t len = (size_t)(end - p);
        if (len >= 1 && len <= out_size - 1) {
            memcpy(out, p, len);
            out[len] = '\0';
            return 1;
        }
        printf("Please enter 1 to %d characters.\n", (int)(out_size - 1));
    }
}

/* Compares two strings like strcmp(), but ignoring upper and lower case.
 * strcmp() compares character codes, and every uppercase letter comes
 * before every lowercase one, so "Smith" would sort before "ben". Here
 * both letters are lowercased first, so "ben" comes first.
 * Returns <0, 0 or >0. */
static int compare_ignore_case(const char *a, const char *b)
{
    for (size_t i = 0;; i++) {
        int ca = tolower((unsigned char)a[i]);
        int cb = tolower((unsigned char)b[i]);
        if (ca != cb || ca == '\0')
            return ca - cb;
    }
}

/* qsort() comparison function: orders two contacts by name, ignoring case.
 * qsort() hands over each element as a const void *, so it is cast back to
 * the real type, const Contact *, before use. */
static int compare_by_name(const void *a, const void *b)
{
    const Contact *x = (const Contact *)a;
    const Contact *y = (const Contact *)b;

    return compare_ignore_case(x->name, y->name);
}

/* qsort() comparison function: orders by email ignoring case, then by name
 * when two emails are the same. */
static int compare_by_email(const void *a, const void *b)
{
    const Contact *x = (const Contact *)a;
    const Contact *y = (const Contact *)b;
    int result = compare_ignore_case(x->email, y->email);

    return result != 0 ? result : compare_ignore_case(x->name, y->name);
}

/* Returns the index of the contact called name (ignoring case), or -1. */
static int find_by_name(const AddressBook *book, const char *name)
{
    for (int i = 0; i < book->count; i++)
        if (compare_ignore_case(book->contacts[i].name, name) == 0)
            return i;
    return -1;
}

/* Returns 1 if phone is 1 to 20 characters of digits, spaces and + - ( ),
 * with at least 3 digits. Deliberately simple. */
static int valid_phone(const char *phone)
{
    size_t len = strlen(phone);
    int digits = 0;

    if (len < 1 || len >= PHONE_SIZE)
        return 0;
    for (size_t i = 0; i < len; i++) {
        if (isdigit((unsigned char)phone[i]))
            digits++;
        else if (strchr(" +-()", phone[i]) == NULL)
            return 0;
    }
    return digits >= 3;
}

/* Returns 1 if email looks like an address: 1 to 50 characters, no spaces,
 * exactly one @ that is neither first nor last, and a . after the @ that is
 * not directly after it, with the address not ending in a dot. This is a
 * simple check: the real rules for email addresses are much harder. */
static int valid_email(const char *email)
{
    size_t len = strlen(email);
    const char *at = strchr(email, '@');

    if (len < 1 || len >= EMAIL_SIZE || strpbrk(email, " \t") != NULL)
        return 0;
    if (at == NULL || at == email || strchr(at + 1, '@') != NULL)
        return 0;                               /* no @, @ first, or two of them */
    if (at[1] == '\0' || at[1] == '.' || email[len - 1] == '.')
        return 0;
    return strchr(at + 1, '.') != NULL;
}

/* Writes one CSV field. A field holding a comma or a quote goes inside
 * quotes, with every " doubled, so the reader knows where it ends. */
static void write_csv_field(FILE *f, const char *field)
{
    if (strpbrk(field, ",\"") == NULL) {
        fputs(field, f);
        return;
    }
    fputc('"', f);
    for (const char *p = field; *p != '\0'; p++) {
        if (*p == '"')
            fputc('"', f);                      /* " is written as "" */
        fputc(*p, f);
    }
    fputc('"', f);
}

/* Splits one CSV line into exactly 3 fields. Splitting at every comma would
 * cut "Smith, John" in two, so a field that starts with " runs to the next
 * single ", and "" inside it means one ". Other fields run to the next comma.
 * Returns 1 for exactly 3 well-formed fields that fit; 0 otherwise. */
static int parse_csv_line(const char *line, char fields[3][EMAIL_SIZE])
{
    const char *p = line;

    for (int f = 0; f < 3; f++) {
        size_t n = 0;
        if (*p == '"') {                        /* a quoted field */
            p++;
            for (;;) {
                if (*p == '\0')
                    return 0;                   /* the quote is never closed */
                if (*p == '"' && p[1] != '"')
                    break;                      /* the closing quote */
                if (*p == '"')
                    p++;                        /* "" stands for one " */
                if (n + 1 >= EMAIL_SIZE)
                    return 0;
                fields[f][n++] = *p++;
            }
            p++;                                /* step over the closing quote */
            if (*p != ',' && *p != '\0')
                return 0;                       /* text after the closing quote */
        } else {                                /* a plain field */
            while (*p != ',' && *p != '\0') {
                if (n + 1 >= EMAIL_SIZE)
                    return 0;
                fields[f][n++] = *p++;
            }
        }
        fields[f][n] = '\0';
        if (f < 2 && *p != ',')
            return 0;                           /* fewer than 3 fields */
        if (f < 2)
            p++;                                /* step over the comma */
    }
    return *p == '\0';                          /* more than 3 fields? */
}

/* Loads the contacts from the CSV file at path and sorts them by name.
 * A missing file means a new, empty address book. Lines that are not valid
 * contacts (wrong field count, unclosed quote, too long, bad phone or
 * email, duplicate name, or past MAX_CONTACTS) are skipped and counted.
 * Returns 0 only if the file exists but cannot be opened. */
static int load_book(AddressBook *book, const char *path)
{
    char line[FILE_LINE], fields[3][EMAIL_SIZE];
    int skipped = 0, line_number = 0;
    FILE *f = fopen(path, "r");

    book->count = 0;
    book->changed = false;
    if (f == NULL) {
        if (errno == ENOENT) {
            printf("No file %s yet: starting a new address book.\n", path);
            return 1;
        }
        printf("Could not open %s: %s\n", path, strerror(errno));
        return 0;
    }
    while (fgets(line, sizeof line, f) != NULL) {
        line_number++;
        if (strchr(line, '\n') == NULL && !feof(f)) {   /* too long: skip the rest */
            int c;
            while ((c = fgetc(f)) != '\n' && c != EOF)
                ;
            skipped++;
            continue;
        }
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0' || (line_number == 1 && strcmp(line, CSV_HEADER) == 0))
            continue;                           /* a blank line or the header */
        if (!parse_csv_line(line, fields) || fields[0][0] == '\0' ||
            strlen(fields[0]) >= NAME_SIZE || !valid_phone(fields[1]) ||
            !valid_email(fields[2]) || find_by_name(book, fields[0]) >= 0 ||
            book->count == MAX_CONTACTS) {
            skipped++;
            continue;
        }
        Contact *c = &book->contacts[book->count++];
        strcpy(c->name, fields[0]);
        strcpy(c->phone, fields[1]);
        strcpy(c->email, fields[2]);
    }
    fclose(f);                                  /* only read from: nothing to lose */
    qsort(book->contacts, (size_t)book->count, sizeof book->contacts[0], compare_by_name);

    printf("Loaded %d contact%s from %s.\n", book->count, book->count == 1 ? "" : "s", path);
    if (skipped > 0)
        printf("(%d line%s skipped: not valid contact lines)\n", skipped, skipped == 1 ? "" : "s");
    return 1;
}

/* Saves every contact to path, safely: write path.tmp, close it and check
 * for errors, then remove the old file and rename the new one into its
 * place. If anything fails halfway, the old file is still intact. The
 * remove() comes first because rename() on Windows fails if the target
 * exists. Returns 1 on success; otherwise prints why and returns 0. */
static int save_book(AddressBook *book, const char *path)
{
    char tmp[PATH_SIZE + 4];
    FILE *f;

    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    f = fopen(tmp, "w");
    if (f == NULL) {
        printf("Could not save: %s\n", strerror(errno));
        return 0;
    }
    fprintf(f, "%s\n", CSV_HEADER);
    for (int i = 0; i < book->count; i++) {
        const Contact *c = &book->contacts[i];
        write_csv_field(f, c->name);
        fputc(',', f);
        write_csv_field(f, c->phone);
        fputc(',', f);
        write_csv_field(f, c->email);
        fputc('\n', f);
    }
    int failed = ferror(f);
    if (fclose(f) != 0 || failed) {
        printf("Could not save: %s\n", strerror(errno));
        remove(tmp);
        return 0;
    }
    if ((remove(path) != 0 && errno != ENOENT) || rename(tmp, path) != 0) {
        printf("Could not save: %s\n", strerror(errno));
        remove(tmp);
        return 0;
    }
    book->changed = false;
    printf("Saved %d contact%s to %s.\n", book->count, book->count == 1 ? "" : "s", path);
    return 1;
}

/* Prints contacts as a table. The number is always the contact's position
 * in name order, found with find_by_name(), so it is the same in the email
 * list and in search results, and is the number Delete asks for. */
static void print_table(const AddressBook *book, const Contact list[], int count)
{
    if (count == 0) {
        printf("The address book is empty.\n");
        return;
    }
    printf("%3s  %-25s %-21s %s\n", "#", "Name", "Phone", "Email");
    for (int i = 0; i < count; i++)
        printf("%3d  %-25s %-21s %s\n", find_by_name(book, list[i].name) + 1,
               list[i].name, list[i].phone, list[i].email);
}

/* Lists the contacts by email. It sorts a copy (a struct is copied with =),
 * so the stored name order, and the numbers Delete uses, never change. */
static void list_by_email(const AddressBook *book)
{
    static Contact copy[MAX_CONTACTS];

    for (int i = 0; i < book->count; i++)
        copy[i] = book->contacts[i];
    qsort(copy, (size_t)book->count, sizeof copy[0], compare_by_email);
    printf("\n");
    print_table(book, copy, book->count);
}

/* Asks for a name, phone and email, each until valid, then adds the contact
 * and keeps the array sorted by name. Returns 0 if input ended, else 1. */
static int add_contact(AddressBook *book)
{
    Contact c;

    if (book->count == MAX_CONTACTS) {
        printf("The address book is full (%d contacts).\n", MAX_CONTACTS);
        return 1;
    }
    for (;;) {
        if (!read_text("Name: ", c.name, NAME_SIZE))
            return 0;
        if (find_by_name(book, c.name) < 0)
            break;
        printf("A contact named %s already exists.\n", c.name);
    }
    for (;;) {
        if (!read_text("Phone: ", c.phone, PHONE_SIZE))
            return 0;
        if (valid_phone(c.phone))
            break;
        printf("Please use digits, spaces and + - ( ) only, with at least 3 digits.\n");
    }
    for (;;) {
        if (!read_text("Email: ", c.email, EMAIL_SIZE))
            return 0;
        if (valid_email(c.email))
            break;
        printf("That does not look like an email address.\n");
    }
    book->contacts[book->count++] = c;
    qsort(book->contacts, (size_t)book->count, sizeof book->contacts[0], compare_by_name);
    book->changed = true;
    printf("Added %s.\n", c.name);
    return 1;
}

/* Returns 1 if word appears anywhere in text, ignoring upper and lower case.
 * strcasestr() and stristr() would do this, but they are not standard C:
 * for each starting place in text, compare the letters of word one by one. */
static int contains_ignore_case(const char *text, const char *word)
{
    for (size_t i = 0; text[i] != '\0'; i++) {
        size_t j = 0;
        while (word[j] != '\0' && text[i + j] != '\0' &&
               tolower((unsigned char)text[i + j]) == tolower((unsigned char)word[j]))
            j++;
        if (word[j] == '\0')
            return 1;                           /* every letter of word matched */
    }
    return 0;
}

/* Asks for a word and lists every contact whose name, phone or email
 * contains it, ignoring case. Returns 0 if input ended, else 1. */
static int search_contacts(const AddressBook *book)
{
    static Contact found[MAX_CONTACTS];
    char word[SEARCH_SIZE];
    int n = 0;

    if (!read_text("Search for: ", word, SEARCH_SIZE))
        return 0;
    for (int i = 0; i < book->count; i++) {
        const Contact *c = &book->contacts[i];
        if (contains_ignore_case(c->name, word) || contains_ignore_case(c->phone, word) ||
            contains_ignore_case(c->email, word))
            found[n++] = *c;
    }
    printf("\n");
    if (n == 0) {
        printf("No contacts match \"%s\".\n", word);
        return 1;
    }
    printf("%d contact%s found:\n", n, n == 1 ? "" : "s");
    print_table(book, found, n);
    return 1;
}

/* Asks for a contact's number and deletes it: every later contact moves
 * down one place, and the count goes down by one. memmove(), not memcpy(),
 * because the source and destination overlap. Returns 0 if input ended. */
static int delete_contact(AddressBook *book)
{
    int n = 0;

    if (book->count == 0) {
        printf("The address book is empty.\n");
        return 1;
    }
    if (!read_int("Delete which number? (0 to cancel): ", 0, book->count, &n))
        return 0;
    if (n == 0) {
        printf("Nothing deleted.\n");
        return 1;
    }
    char name[NAME_SIZE];
    strcpy(name, book->contacts[n - 1].name);
    memmove(&book->contacts[n - 1], &book->contacts[n],
            (size_t)(book->count - n) * sizeof book->contacts[0]);
    book->count--;
    book->changed = true;
    printf("Deleted %s.\n", name);
    return 1;
}

/* Prints prompt and reads y, yes, n or no in any case, with spaces around
 * it allowed. Anything else asks again.
 * Returns 1 for yes, 0 for no, -1 at end of input. */
static int ask_yes_no(const char *prompt)
{
    char buf[LINE_SIZE];

    for (;;) {
        char *p = buf, *end;

        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return -1;
        while (*p == ' ' || *p == '\t')
            p++;
        end = p + strlen(p);
        while (end > p && (end[-1] == ' ' || end[-1] == '\t'))
            end--;
        *end = '\0';                            /* cut the trailing spaces */
        for (char *q = p; *q != '\0'; q++)
            *q = (char)tolower((unsigned char)*q);
        if (strcmp(p, "y") == 0 || strcmp(p, "yes") == 0)
            return 1;
        if (strcmp(p, "n") == 0 || strcmp(p, "no") == 0)
            return 0;
        printf("Please answer y or n.\n");
    }
}
