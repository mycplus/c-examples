/*
 * library_management.c - a small library desk: books, members and loans.
 *
 * Books have an ID (101, 102...), a title, an author and a number of copies.
 * Members have an ID (1, 2...) and a name. Issuing a book creates a loan,
 * due back in 14 days; a late return is fined 0.25 per day. The loans
 * report lists every loan, soonest due first. Every change is saved at once
 * to one text file with one record per line, its first letter saying which
 * kind it is:
 *     B|101|2|Learning C Step by Step|Ana Writer     a book: id, copies, title, author
 *     M|1|Ana Lopez                                  a member: id, name
 *     L|101|1|2026-12-15                             a loan: book id, member id, due date
 *
 * Reused from earlier projects, copied unchanged:
 *   read_line, read_int                     unit_converter.c     (project 1)
 *   read_text                               grade_report.c       (project 5)
 *   is_leap_year, is_valid_date,
 *   today_string                            personal_diary.c     (project 10)
 *   Date, days_before_month, parse_date,
 *   compare_dates, day_of_year              birthday_reminder.c  (project 12)
 *
 * Build with GCC or Clang:
 *     gcc -std=c11 -Wall -Wextra -o library_management library_management.c
 * Build with Visual Studio (Developer Command Prompt):
 *     cl /W4 library_management.c
 * Run:
 *     ./library_management                       uses library.txt and today's date
 *     ./library_management --today 2027-01-10    see overdue loans
 */
#define _CRT_SECURE_NO_WARNINGS   /* fopen, strerror, localtime, remove, rename on MSVC */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LINE_SIZE      64         /* read_line buffer (read_text uses it too) */
#define TITLE_SIZE     31         /* 30 characters + '\0' */
#define AUTHOR_SIZE    21
#define NAME_SIZE      21
#define DATE_SIZE      11
#define FILE_LINE      128
#define PATH_SIZE      260
#define MAX_BOOKS      200
#define MAX_MEMBERS    100
#define MAX_LOANS      300
#define MAX_PER_MEMBER 3
#define MAX_COPIES     99
#define MAX_ID         9999
#define LOAN_DAYS      14
#define FINE_CENTS     25         /* per day late */
#define FIRST_BOOK_ID  101
#define FIRST_MEMBER_ID 1
#define DEFAULT_FILE   "library.txt"

/* A calendar date. It is only three ints, so it is passed and returned by
 * value everywhere: copying it is cheap and avoids pointer mistakes. */
typedef struct {
    int year;
    int month;
    int day;
} Date;

/* A book. "Available copies" is not stored: copies_available() counts the
 * loans each time, so it can never drift out of step with them. */
typedef struct {
    int  id;
    char title[TITLE_SIZE];
    char author[AUTHOR_SIZE];
    int  copies;
} Book;

/* A library member. */
typedef struct {
    int  id;
    char name[NAME_SIZE];
} Member;

/* A loan links a book and a member by their IDs, like a row in a database
 * table. Not a copy of the title or name: a copy would go stale if the name
 * changed. Not a pointer into the arrays: removing a member shifts the
 * array, and the pointer would then point at the wrong one. An ID stays
 * valid, and it can be written to the file. */
typedef struct {
    int  book_id;             /* links to Book.id */
    int  member_id;           /* links to Member.id */
    Date due;
} Loan;

/* The whole library: three arrays and their counts. */
typedef struct {
    Book   books[MAX_BOOKS];
    int    book_count;
    Member members[MAX_MEMBERS];
    int    member_count;
    Loan   loans[MAX_LOANS];
    int    loan_count;
} Library;

/* Days in the year before the first of each month (not a leap year). */
static const int days_before_month[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

static int  read_line(char *buf, size_t size);
static int  read_int(const char *prompt, int lo, int hi, int *value);
static int  read_text(const char *prompt, char *out, size_t out_size);
static int  is_leap_year(int year);
static int  is_valid_date(const char *text);
static void today_string(char out[DATE_SIZE]);
static Date parse_date(const char *text);
static int  compare_dates(Date a, Date b);
static int  day_of_year(Date d);
static int  month_length(int year, int month);
static int  day_number(Date d);
static Date add_days(Date d, int days);
static int  find_book(const Library *lib, int id);
static int  find_member(const Library *lib, int id);
static int  find_loan(const Library *lib, int book_id, int member_id);
static int  copies_available(const Library *lib, int book_id);
static int  loans_of_member(const Library *lib, int member_id);
static int  next_book_id(const Library *lib);
static int  next_member_id(const Library *lib);
static int  parse_whole(const char *text, int lo, int hi, int *value);
static int  split_fields(char *line, char *fields[], int max);
static int  check_loans(Library *lib);
static int  load_library(Library *lib, const char *path);
static int  save_library(const Library *lib, const char *path);
static int  compare_loans(const void *a, const void *b);
static void list_books(const Library *lib);
static void list_members(const Library *lib);
static void list_loans(const Library *lib, Date today);
static int  read_name(const char *prompt, char *out, size_t size);
static int  add_book(Library *lib, const char *path);
static int  add_member(Library *lib, const char *path);
static int  issue_book(Library *lib, const char *path, Date today);
static int  return_book(Library *lib, const char *path, Date today);
static int  remove_member(Library *lib, const char *path);

int main(int argc, char *argv[])
{
    static Library lib;                         /* static: about 21 KB, kept off the stack */
    const char *path = DEFAULT_FILE;
    char today_text[DATE_SIZE] = "";
    int choice = 0, ok = 1;

    for (int i = 1; i < argc; i += 2) {         /* options come in pairs, any order */
        if (i + 1 < argc && strcmp(argv[i], "--file") == 0 && strlen(argv[i + 1]) < PATH_SIZE - 4) {
            path = argv[i + 1];
        } else if (i + 1 < argc && strcmp(argv[i], "--today") == 0 && is_valid_date(argv[i + 1]) &&
                   parse_date(argv[i + 1]).year >= 1900 && parse_date(argv[i + 1]).year <= 9998) {
            strcpy(today_text, argv[i + 1]);    /* 9998 keeps every due date a 4-digit year */
        } else {
            fprintf(stderr, "Usage: library_management [--file NAME] [--today YYYY-MM-DD]\n");
            return 1;
        }
    }
    if (today_text[0] == '\0')
        today_string(today_text);
    Date today = parse_date(today_text);

    printf("Library Management\n==================\nToday is %s.\n", today_text);
    if (!load_library(&lib, path))
        return 1;
    int overdue = 0;
    for (int i = 0; i < lib.loan_count; i++)
        if (compare_dates(lib.loans[i].due, today) < 0)
            overdue++;
    if (overdue > 0)
        printf("Overdue: %d loan%s. Choose 7 to see them.\n", overdue, overdue == 1 ? "" : "s");

    while (ok) {
        printf("\n1. List books\n2. List members\n3. Add a book\n4. Add a member\n"
               "5. Issue a book\n6. Return a book\n7. Loans and overdue books\n"
               "8. Remove a member\n0. Quit\n");
        if (!read_int("Choose an option (0-8): ", 0, 8, &choice))
            break;
        switch (choice) {
        case 1: printf("\n"); list_books(&lib); break;
        case 2: printf("\n"); list_members(&lib); break;
        case 3: ok = add_book(&lib, path); break;
        case 4: ok = add_member(&lib, path); break;
        case 5: ok = issue_book(&lib, path, today); break;
        case 6: ok = return_book(&lib, path, today); break;
        case 7: printf("\n"); list_loans(&lib, today); break;
        case 8: ok = remove_member(&lib, path); break;
        default:
            printf("\nGoodbye!\n");
            return 0;
        }
    }
    printf("\n\nGoodbye!\n");                  /* input ended; every change is already saved */
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

/* Returns 1 if year is a leap year, 0 otherwise. A year is a leap year if
 * it is divisible by 4, except years divisible by 100, unless they are
 * also divisible by 400: 2024 and 2000 are leap years, 1900 and 2026 not. */
static int is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

/* Returns 1 if text is a real date written YYYY-MM-DD: exactly 10
 * characters, digits and two dashes in the right places, a month from 1
 * to 12 and a day that exists in that month (29 February only in a leap
 * year). Returns 0 otherwise. */
static int is_valid_date(const char *text)
{
    static const int days_in_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (strlen(text) != 10 || text[4] != '-' || text[7] != '-')
        return 0;
    for (int i = 0; i < 10; i++)
        if (i != 4 && i != 7 && !isdigit((unsigned char)text[i]))
            return 0;

    /* Every character is a checked digit, so c - '0' gives its value. */
    int year  = (text[0] - '0') * 1000 + (text[1] - '0') * 100 + (text[2] - '0') * 10 + (text[3] - '0');
    int month = (text[5] - '0') * 10 + (text[6] - '0');
    int day   = (text[8] - '0') * 10 + (text[9] - '0');
    if (month < 1 || month > 12)
        return 0;
    int last = days_in_month[month - 1];
    if (month == 2 && is_leap_year(year))
        last = 29;
    return day >= 1 && day <= last;
}

/* Writes today's date from the computer's clock into out as YYYY-MM-DD. */
static void today_string(char out[DATE_SIZE])
{
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    if (local == NULL || strftime(out, DATE_SIZE, "%Y-%m-%d", local) == 0)
        strcpy(out, "1970-01-01");              /* the clock could not be read */
}

/* Turns "YYYY-MM-DD" into a Date. The text must already have passed
 * is_valid_date(), so every character used is a digit. */
static Date parse_date(const char *text)
{
    Date d;

    d.year  = (text[0] - '0') * 1000 + (text[1] - '0') * 100 + (text[2] - '0') * 10 + (text[3] - '0');
    d.month = (text[5] - '0') * 10 + (text[6] - '0');
    d.day   = (text[8] - '0') * 10 + (text[9] - '0');
    return d;
}

/* Compares two dates by year, then month, then day.
 * Returns <0 if a is earlier, 0 if they are the same day, >0 if a is later. */
static int compare_dates(Date a, Date b)
{
    if (a.year != b.year)
        return a.year - b.year;
    if (a.month != b.month)
        return a.month - b.month;
    return a.day - b.day;
}

/* Returns the day of the year, 1 to 366: the days before the month, from
 * the table, plus the day, plus 1 after February in a leap year. */
static int day_of_year(Date d)
{
    int n = days_before_month[d.month - 1] + d.day;

    if (d.month > 2 && is_leap_year(d.year))
        n++;
    return n;
}

/* Returns the number of days in month (1 to 12) of year: 29 for February
 * in a leap year. It repeats the table inside the copied is_valid_date(),
 * which stays exactly as it was in the diary. */
static int month_length(int year, int month)
{
    static const int lengths[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (month == 2 && is_leap_year(year))
        return 29;
    return lengths[month - 1];
}

/* Returns the number of days from 1900-01-01 to d (0 for 1900-01-01).
 * With a day number for each date, the days between any two dates is one
 * subtraction: simpler than the Birthday Reminder's same-year and next-year
 * cases, and it works for any two dates. A loop over the years is fast
 * enough and easier to follow than a formula. */
static int day_number(Date d)
{
    int n = 0;

    for (int year = 1900; year < d.year; year++)
        n += is_leap_year(year) ? 366 : 365;
    return n + day_of_year(d) - 1;
}

/* Returns d moved forward by days, a month at a time: while the day is past
 * the end of its month, take away that month's length and move to the next
 * month, rolling December into January. So 2026-12-20 + 14 is 2027-01-03. */
static Date add_days(Date d, int days)
{
    d.day += days;
    while (d.day > month_length(d.year, d.month)) {
        d.day -= month_length(d.year, d.month);
        if (++d.month > 12) {
            d.month = 1;
            d.year++;
        }
    }
    return d;
}

/* Returns the index of the book with this ID, or -1. */
static int find_book(const Library *lib, int id)
{
    for (int i = 0; i < lib->book_count; i++)
        if (lib->books[i].id == id)
            return i;
    return -1;
}

/* Returns the index of the member with this ID, or -1. */
static int find_member(const Library *lib, int id)
{
    for (int i = 0; i < lib->member_count; i++)
        if (lib->members[i].id == id)
            return i;
    return -1;
}

/* Returns the index of the loan of book_id to member_id, or -1. */
static int find_loan(const Library *lib, int book_id, int member_id)
{
    for (int i = 0; i < lib->loan_count; i++)
        if (lib->loans[i].book_id == book_id && lib->loans[i].member_id == member_id)
            return i;
    return -1;
}

/* Returns how many copies of the book are on the shelf: its copies minus
 * its loans, counted now rather than stored, so it is always right. */
static int copies_available(const Library *lib, int book_id)
{
    int i = find_book(lib, book_id), available = (i < 0) ? 0 : lib->books[i].copies;

    for (int k = 0; k < lib->loan_count; k++)
        if (lib->loans[k].book_id == book_id)
            available--;
    return available;
}

/* Returns how many books the member has on loan, counted from the loans. */
static int loans_of_member(const Library *lib, int member_id)
{
    int n = 0;

    for (int k = 0; k < lib->loan_count; k++)
        if (lib->loans[k].member_id == member_id)
            n++;
    return n;
}

/* Returns the ID for a new book: the highest ID plus 1, or FIRST_BOOK_ID.
 * IDs are not stored anywhere else, so after a restart the ID of a removed
 * record can be given out again. That is harmless here, because a member
 * with loans cannot be removed; a real system would keep the counter in
 * the file. */
static int next_book_id(const Library *lib)
{
    int id = FIRST_BOOK_ID - 1;

    for (int i = 0; i < lib->book_count; i++)
        if (lib->books[i].id > id)
            id = lib->books[i].id;
    return id + 1;
}

/* Returns the ID for a new member, as next_book_id() does for books. */
static int next_member_id(const Library *lib)
{
    int id = FIRST_MEMBER_ID - 1;

    for (int i = 0; i < lib->member_count; i++)
        if (lib->members[i].id > id)
            id = lib->members[i].id;
    return id + 1;
}

/* Reads text, a field from the file, as a whole number from lo to hi.
 * Returns 1 and sets *value if it is one, 0 otherwise. */
static int parse_whole(const char *text, int lo, int hi, int *value)
{
    char *end;
    long n;

    if (!isdigit((unsigned char)text[0]))
        return 0;                               /* no sign, no spaces */
    errno = 0;
    n = strtol(text, &end, 10);
    if (*end != '\0' || errno == ERANGE || n < lo || n > hi)
        return 0;
    *value = (int)n;
    return 1;
}

/* Splits line at every '|', writing '\0' over each one, and stores up to
 * max field pointers in fields[]. Returns how many fields the line has,
 * which may be more than max. Splitting at every '|' is safe because no
 * text may contain one: simpler than the address book's CSV quoting, but
 * less flexible. */
static int split_fields(char *line, char *fields[], int max)
{
    int n = 0;

    fields[n++] = line;
    for (char *p = line; *p != '\0'; p++) {
        if (*p == '|') {
            *p = '\0';
            if (n < max)
                fields[n] = p + 1;
            n++;
        }
    }
    return n;
}

/* Keeps only the loans that obey the rules: the book and the member exist,
 * the pair has not appeared before, the member has fewer than 3 loans and
 * the book still has a free copy. Loans are checked in file order, after
 * the whole file has been read, so the order of the lines does not matter.
 * Returns how many loans were dropped. */
static int check_loans(Library *lib)
{
    int total = lib->loan_count, dropped = 0;

    lib->loan_count = 0;                        /* rebuild the list in place */
    for (int i = 0; i < total; i++) {
        Loan loan = lib->loans[i];
        if (find_book(lib, loan.book_id) >= 0 && find_member(lib, loan.member_id) >= 0 &&
            find_loan(lib, loan.book_id, loan.member_id) < 0 &&
            loans_of_member(lib, loan.member_id) < MAX_PER_MEMBER &&
            copies_available(lib, loan.book_id) > 0)
            lib->loans[lib->loan_count++] = loan;
        else
            dropped++;
    }
    return dropped;
}

/* Loads the books, members and loans from the file at path. A missing file
 * means a new, empty library. Lines that are not valid records are skipped
 * and counted, together with the loans check_loans() drops. Returns 0 only
 * if the file exists but cannot be opened. */
static int load_library(Library *lib, const char *path)
{
    char line[FILE_LINE];
    char *f[5];
    int skipped = 0, a = 0, b = 0;
    FILE *file = fopen(path, "r");

    lib->book_count = lib->member_count = lib->loan_count = 0;
    if (file == NULL) {
        if (errno == ENOENT) {
            printf("No file %s yet: starting a new library.\n", path);
            return 1;
        }
        printf("Could not open %s: %s\n", path, strerror(errno));
        return 0;
    }
    while (fgets(line, sizeof line, file) != NULL) {
        if (strchr(line, '\n') == NULL && !feof(file)) {    /* too long: skip the rest */
            int c;
            while ((c = fgetc(file)) != '\n' && c != EOF)
                ;
            skipped++;
            continue;
        }
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0')
            continue;                           /* a blank line */
        int n = split_fields(line, f, 5);
        if (strcmp(f[0], "B") == 0 && n == 5 && parse_whole(f[1], 1, MAX_ID, &a) &&
            parse_whole(f[2], 1, MAX_COPIES, &b) && f[3][0] != '\0' && strlen(f[3]) < TITLE_SIZE &&
            f[4][0] != '\0' && strlen(f[4]) < AUTHOR_SIZE && find_book(lib, a) < 0 &&
            lib->book_count < MAX_BOOKS) {
            Book *book = &lib->books[lib->book_count++];
            book->id = a;
            book->copies = b;
            strcpy(book->title, f[3]);
            strcpy(book->author, f[4]);
        } else if (strcmp(f[0], "M") == 0 && n == 3 && parse_whole(f[1], 1, MAX_ID, &a) &&
                   f[2][0] != '\0' && strlen(f[2]) < NAME_SIZE && find_member(lib, a) < 0 &&
                   lib->member_count < MAX_MEMBERS) {
            lib->members[lib->member_count].id = a;
            strcpy(lib->members[lib->member_count++].name, f[2]);
        } else if (strcmp(f[0], "L") == 0 && n == 4 && parse_whole(f[1], 1, MAX_ID, &a) &&
                   parse_whole(f[2], 1, MAX_ID, &b) && is_valid_date(f[3]) &&
                   parse_date(f[3]).year >= 1900 && lib->loan_count < MAX_LOANS) {
            Loan *loan = &lib->loans[lib->loan_count++];
            loan->book_id = a;
            loan->member_id = b;
            loan->due = parse_date(f[3]);
        } else {
            skipped++;
        }
    }
    fclose(file);                               /* only read from: nothing to lose */
    skipped += check_loans(lib);

    printf("Loaded %d book%s, %d member%s and %d loan%s from %s.\n",
           lib->book_count, lib->book_count == 1 ? "" : "s",
           lib->member_count, lib->member_count == 1 ? "" : "s",
           lib->loan_count, lib->loan_count == 1 ? "" : "s", path);
    if (skipped > 0)
        printf("(%d line%s skipped: not valid library lines)\n", skipped, skipped == 1 ? "" : "s");
    return 1;
}

/* Saves the books, then the members, then the loans, the safe way: write
 * path.tmp, close it, then rename the new file over the old one (removing
 * the old one first only where rename requires it, as on Windows). Prints
 * nothing on success. Returns 1 on success; otherwise prints why and 0. */
static int save_library(const Library *lib, const char *path)
{
    char tmp[PATH_SIZE + 4];
    FILE *f;

    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    f = fopen(tmp, "w");
    if (f == NULL) {
        printf("Could not save: %s\n", strerror(errno));
        return 0;
    }
    for (int i = 0; i < lib->book_count; i++)
        fprintf(f, "B|%d|%d|%s|%s\n", lib->books[i].id, lib->books[i].copies,
                lib->books[i].title, lib->books[i].author);
    for (int i = 0; i < lib->member_count; i++)
        fprintf(f, "M|%d|%s\n", lib->members[i].id, lib->members[i].name);
    for (int i = 0; i < lib->loan_count; i++) {
        const Loan *l = &lib->loans[i];
        fprintf(f, "L|%d|%d|%04d-%02d-%02d\n", l->book_id, l->member_id,
                l->due.year, l->due.month, l->due.day);
    }
    int failed = ferror(f);
    if (fclose(f) != 0 || failed) {
        printf("Could not save: %s\n", strerror(errno));
        remove(tmp);
        return 0;
    }
    /* rename() replaces the old file in one step on Linux and macOS. On
     * Windows it fails if the old file exists, so remove that and try
     * again. If the second try fails too, FILE.tmp is now the only copy
     * of the data, so it is kept, never deleted. */
    if (rename(tmp, path) != 0) {
        remove(path);
        if (rename(tmp, path) != 0) {
            printf("Could not save: %s. Your records are in %s.\n", strerror(errno), tmp);
            return 0;
        }
    }
    return 1;
}

/* qsort() comparison for loans: due date, then book ID, then member ID. */
static int compare_loans(const void *a, const void *b)
{
    const Loan *x = (const Loan *)a;
    const Loan *y = (const Loan *)b;
    int by_date = compare_dates(x->due, y->due);

    if (by_date != 0)
        return by_date;
    if (x->book_id != y->book_id)
        return x->book_id - y->book_id;
    return x->member_id - y->member_id;
}

/* Prints every book in ID order, with its copies and how many are free. */
static void list_books(const Library *lib)
{
    if (lib->book_count == 0) {
        printf("No books yet.\n");
        return;
    }
    printf("%4s  %-30s %-20s %6s %9s\n", "ID", "Title", "Author", "Copies", "Available");
    for (int i = 0; i < lib->book_count; i++) {
        const Book *b = &lib->books[i];
        printf("%4d  %-30s %-20s %6d %9d\n", b->id, b->title, b->author, b->copies,
               copies_available(lib, b->id));
    }
}

/* Prints every member in ID order, with how many books they have on loan. */
static void list_members(const Library *lib)
{
    if (lib->member_count == 0) {
        printf("No members yet.\n");
        return;
    }
    printf("%4s  %-20s %s\n", "ID", "Name", "Books on loan");
    for (int i = 0; i < lib->member_count; i++)
        printf("%4d  %-20s %d\n", lib->members[i].id, lib->members[i].name,
               loans_of_member(lib, lib->members[i].id));
}

/* Prints every loan, soonest due first, with the title and member name
 * looked up by ID and whether it is due or overdue. A copy of the loans is
 * sorted, so the stored list is left as it is. */
static void list_loans(const Library *lib, Date today)
{
    static Loan sorted[MAX_LOANS];
    char status[32];
    int overdue = 0;

    if (lib->loan_count == 0) {
        printf("No books are on loan.\n");
        return;
    }
    for (int i = 0; i < lib->loan_count; i++) {
        sorted[i] = lib->loans[i];
        if (compare_dates(sorted[i].due, today) < 0)
            overdue++;
    }
    qsort(sorted, (size_t)lib->loan_count, sizeof sorted[0], compare_loans);

    printf("%d loan%s, %d overdue:\n", lib->loan_count, lib->loan_count == 1 ? "" : "s", overdue);
    printf("%-10s  %4s  %-30s %-20s %s\n", "Due", "Book", "Title", "Member", "Status");
    for (int i = 0; i < lib->loan_count; i++) {
        const Loan *l = &sorted[i];
        int days = day_number(l->due) - day_number(today);
        if (days == 0)
            strcpy(status, "due today");
        else if (days > 0)
            snprintf(status, sizeof status, "due in %d day%s", days, days == 1 ? "" : "s");
        else
            snprintf(status, sizeof status, "overdue by %d day%s", -days, days == -1 ? "" : "s");
        printf("%04d-%02d-%02d  %4d  %-30s %-20s %s\n", l->due.year, l->due.month, l->due.day, l->book_id,
               lib->books[find_book(lib, l->book_id)].title,
               lib->members[find_member(lib, l->member_id)].name, status);
    }
}

/* Reads a title, author or name with read_text(), asking again if it holds
 * a |, the file's separator. Returns 1 on success, 0 at end of input. */
static int read_name(const char *prompt, char *out, size_t size)
{
    for (;;) {
        if (!read_text(prompt, out, size))
            return 0;
        if (strchr(out, '|') == NULL)
            return 1;
        printf("Please do not use the | character.\n");
    }
}

/* Asks for a title, author and number of copies, adds the book and saves.
 * Returns 0 if input ended, else 1. */
static int add_book(Library *lib, const char *path)
{
    Book b;

    if (lib->book_count == MAX_BOOKS) {
        printf("The library is full (%d books).\n", MAX_BOOKS);
        return 1;
    }
    if (!read_name("Title: ", b.title, TITLE_SIZE) || !read_name("Author: ", b.author, AUTHOR_SIZE) ||
        !read_int("Copies (1-99): ", 1, MAX_COPIES, &b.copies))
        return 0;
    b.id = next_book_id(lib);
    lib->books[lib->book_count++] = b;
    printf("Added book %d.\n", b.id);
    save_library(lib, path);
    return 1;
}

/* Asks for a name, adds the member and saves. Returns 0 if input ended. */
static int add_member(Library *lib, const char *path)
{
    Member m;

    if (lib->member_count == MAX_MEMBERS) {
        printf("The member list is full (%d members).\n", MAX_MEMBERS);
        return 1;
    }
    if (!read_name("Name: ", m.name, NAME_SIZE))
        return 0;
    m.id = next_member_id(lib);
    lib->members[lib->member_count++] = m;
    printf("Added member %d.\n", m.id);
    save_library(lib, path);
    return 1;
}

/* Lends a book to a member if every rule allows it: both exist, the member
 * does not already have this book or 3 books, and a copy is free. The loan
 * is due back in LOAN_DAYS days. Returns 0 if input ended, else 1. */
static int issue_book(Library *lib, const char *path, Date today)
{
    int book_id = 0, member_id = 0;

    if (!read_int("Book ID: ", 1, MAX_ID, &book_id))
        return 0;
    if (find_book(lib, book_id) < 0) {
        printf("There is no book %d.\n", book_id);
        return 1;
    }
    if (!read_int("Member ID: ", 1, MAX_ID, &member_id))
        return 0;
    int m = find_member(lib, member_id);
    if (m < 0) {
        printf("There is no member %d.\n", member_id);
        return 1;
    }
    const char *name = lib->members[m].name;
    if (find_loan(lib, book_id, member_id) >= 0)
        printf("%s already has book %d.\n", name, book_id);
    else if (loans_of_member(lib, member_id) >= MAX_PER_MEMBER)
        printf("%s already has %d books, the limit.\n", name, MAX_PER_MEMBER);
    else if (copies_available(lib, book_id) <= 0)
        printf("No copies of book %d are available.\n", book_id);
    else if (lib->loan_count == MAX_LOANS)
        printf("The loan list is full (%d loans).\n", MAX_LOANS);
    else {
        Loan *loan = &lib->loans[lib->loan_count++];
        loan->book_id = book_id;
        loan->member_id = member_id;
        loan->due = add_days(today, LOAN_DAYS);
        printf("Issued book %d to %s. Due back on %04d-%02d-%02d.\n", book_id, name,
               loan->due.year, loan->due.month, loan->due.day);
        save_library(lib, path);
    }
    return 1;
}

/* Takes a book back from a member and charges FINE_CENTS for each day
 * late, kept in whole cents as in the Profit and Loss Calculator. Loans
 * are sorted every time they are shown, so their stored order does not
 * matter and the last loan simply moves into the gap. Returns 0 if input
 * ended, else 1. */
static int return_book(Library *lib, const char *path, Date today)
{
    int book_id = 0, member_id = 0;

    if (!read_int("Book ID: ", 1, MAX_ID, &book_id))
        return 0;
    if (find_book(lib, book_id) < 0) {
        printf("There is no book %d.\n", book_id);
        return 1;
    }
    if (!read_int("Member ID: ", 1, MAX_ID, &member_id))
        return 0;
    int m = find_member(lib, member_id);
    if (m < 0) {
        printf("There is no member %d.\n", member_id);
        return 1;
    }
    int i = find_loan(lib, book_id, member_id);
    if (i < 0) {
        printf("%s does not have book %d.\n", lib->members[m].name, book_id);
        return 1;
    }
    int late = day_number(today) - day_number(lib->loans[i].due);
    lib->loans[i] = lib->loans[--lib->loan_count];
    if (late <= 0) {
        printf("%s returned book %d on time.\n", lib->members[m].name, book_id);
    } else {
        int fine = late * FINE_CENTS;
        printf("%s returned book %d %d day%s late. Fine: %d.%02d\n", lib->members[m].name,
               book_id, late, late == 1 ? "" : "s", fine / 100, fine % 100);
    }
    save_library(lib, path);
    return 1;
}

/* Removes a member, but only one with no books on loan: otherwise their
 * loans would point at nobody. Members are listed in ID order, so the later
 * members move down with memmove to keep that order. Returns 0 if input
 * ended, else 1. */
static int remove_member(Library *lib, const char *path)
{
    int member_id = 0;

    if (!read_int("Member ID: ", 1, MAX_ID, &member_id))
        return 0;
    int m = find_member(lib, member_id);
    if (m < 0) {
        printf("There is no member %d.\n", member_id);
        return 1;
    }
    int on_loan = loans_of_member(lib, member_id);
    if (on_loan > 0) {
        printf("%s still has %d book%s on loan and cannot be removed.\n",
               lib->members[m].name, on_loan, on_loan == 1 ? "" : "s");
        return 1;
    }
    printf("Removed member %d (%s).\n", member_id, lib->members[m].name);
    memmove(&lib->members[m], &lib->members[m + 1],
            (size_t)(lib->member_count - m - 1) * sizeof lib->members[0]);
    lib->member_count--;
    save_library(lib, path);
    return 1;
}
