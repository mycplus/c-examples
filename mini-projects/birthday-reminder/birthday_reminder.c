/*
 * birthday_reminder.c - birthdays kept in a text file, with reminders.
 *
 * At start-up it tells you whose birthday is today and whose is coming up
 * in the next 7 days, with the age each person turns. The full list shows
 * everyone, soonest birthday first. Add and delete save the file straight
 * away. Each line of the file is one person:  1990-12-01 | Ana Lopez
 * People born on 29 February celebrate on 28 February in other years.
 *
 * Reused from earlier projects, copied unchanged:
 *   read_line, read_int                     unit_converter.c  (project 1)
 *   read_text                               grade_report.c    (project 5)
 *   is_leap_year, is_valid_date,
 *   today_string, split_entry               personal_diary.c  (project 10)
 *   compare_ignore_case                     address_book.c    (project 11)
 *
 * Why not mktime()? It works in local time, so a daylight-saving change can
 * make a "day" 23 or 25 hours long, and dividing seconds by 86400 can then
 * be off by one. Counting whole days with integers has no such problem and
 * gives the same answer on every computer.
 *
 * Build with GCC or Clang:
 *     gcc -std=c11 -Wall -Wextra -o birthday_reminder birthday_reminder.c
 * Build with Visual Studio (Developer Command Prompt):
 *     cl /W4 birthday_reminder.c
 * Run:
 *     ./birthday_reminder                      uses birthdays.txt and today's date
 *     ./birthday_reminder --today 2027-02-28   try another date
 */
#define _CRT_SECURE_NO_WARNINGS   /* fopen, strerror, localtime, remove, rename on MSVC */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LINE_SIZE    64           /* read_line buffer (read_text uses it too) */
#define NAME_SIZE    41           /* 40 characters + '\0' */
#define DATE_SIZE    11           /* "YYYY-MM-DD" + '\0' */
#define WHEN_SIZE    16           /* "in 365 days" + '\0' fits */
#define FILE_LINE    128          /* one line of the file */
#define PATH_SIZE    260
#define MAX_PEOPLE   100
#define REMIND_DAYS  7            /* start-up reminder window: today + 7 days */
#define FIRST_YEAR   1900
#define DEFAULT_FILE "birthdays.txt"
#define SEPARATOR    " | "

/* A calendar date. It is only three ints, so it is passed and returned by
 * value everywhere: copying it is cheap and avoids pointer mistakes. */
typedef struct {
    int year;
    int month;
    int day;
} Date;

/* One person and their date of birth. */
typedef struct {
    char name[NAME_SIZE];
    Date birth;
} Person;

/* Everyone, in no particular order: the list is sorted each time it is shown. */
typedef struct {
    Person people[MAX_PEOPLE];    /* in no particular order */
    int    count;
} BirthdayList;

/* One person's next birthday, worked out once so it can be sorted. */
typedef struct {
    const Person *person;
    Date          next;           /* the date it is celebrated */
    int           days;           /* 0 = today */
    int           turns;          /* age on that date */
} Upcoming;

/* Days in the year before the first of each month (not a leap year). */
static const int days_before_month[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

static int  read_line(char *buf, size_t size);
static int  read_int(const char *prompt, int lo, int hi, int *value);
static int  read_text(const char *prompt, char *out, size_t out_size);
static int  is_leap_year(int year);
static int  is_valid_date(const char *text);
static void today_string(char out[DATE_SIZE]);
static int  split_entry(char *line, char **date, char **text);
static int  compare_ignore_case(const char *a, const char *b);
static Date parse_date(const char *text);
static int  compare_dates(Date a, Date b);
static int  day_of_year(Date d);
static int  celebration_day(int month, int day, int year);
static Date next_birthday(Date birth, Date today);
static int  days_until(Date today, Date next);
static int  is_valid_birth_date(const char *text, Date today, Date *out);
static int  read_date(const char *prompt, Date today, Date *out);
static int  find_by_name(const BirthdayList *list, const char *name);
static int  compare_upcoming(const void *a, const void *b);
static int  build_upcoming(const BirthdayList *list, Date today, Upcoming out[]);
static void format_when(int days, char out[WHEN_SIZE]);
static void show_reminders(const BirthdayList *list, Date today);
static void list_all(const BirthdayList *list, Date today);
static int  load_list(BirthdayList *list, const char *path, Date today);
static int  save_list(const BirthdayList *list, const char *path);
static int  add_birthday(BirthdayList *list, const char *path, Date today);
static int  delete_birthday(BirthdayList *list, const char *path);

int main(int argc, char *argv[])
{
    static BirthdayList list;                   /* static: about 6 KB, kept off the stack */
    const char *path = DEFAULT_FILE;
    char today_text[DATE_SIZE] = "";
    int choice = 0, ok = 1;

    for (int i = 1; i < argc; i += 2) {         /* options come in pairs, any order */
        if (i + 1 < argc && strcmp(argv[i], "--file") == 0 && strlen(argv[i + 1]) < PATH_SIZE - 4) {
            path = argv[i + 1];
        } else if (i + 1 < argc && strcmp(argv[i], "--today") == 0 && is_valid_date(argv[i + 1])) {
            strcpy(today_text, argv[i + 1]);
        } else {
            fprintf(stderr, "Usage: birthday_reminder [--file NAME] [--today YYYY-MM-DD]\n");
            return 1;
        }
    }
    if (today_text[0] == '\0')
        today_string(today_text);
    Date today = parse_date(today_text);

    printf("Birthday Reminder\n=================\nToday is %s.\n", today_text);
    if (!load_list(&list, path, today))
        return 1;
    printf("\n");
    show_reminders(&list, today);

    /* Every change is saved at once, so end of input loses nothing. */
    while (ok) {
        printf("\n1. Show reminders\n2. List all birthdays\n3. Add a birthday\n"
               "4. Delete a birthday\n0. Quit\n");
        if (!read_int("Choose an option (0-4): ", 0, 4, &choice))
            break;
        switch (choice) {
        case 1: printf("\n"); show_reminders(&list, today); break;
        case 2: printf("\n"); list_all(&list, today); break;
        case 3: ok = add_birthday(&list, path, today); break;
        case 4: ok = delete_birthday(&list, path); break;
        default:
            printf("\nGoodbye!\n");
            return 0;
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

/* Splits a diary line at the first " | " into *date and *text, by writing
 * '\0' over the separator. A text may contain " | " itself: only the first
 * one splits. Returns 1 if the line has the separator, 0 otherwise. */
static int split_entry(char *line, char **date, char **text)
{
    char *sep = strstr(line, SEPARATOR);

    if (sep == NULL)
        return 0;
    *sep = '\0';
    *date = line;
    *text = sep + strlen(SEPARATOR);
    return 1;
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

/* Returns the day of the month on which a birthday of month/day is
 * celebrated in year. Someone born on 29 February celebrates on 28 February
 * when year is not a leap year. (Some countries use 1 March instead; this
 * program picks 28 February.) */
static int celebration_day(int month, int day, int year)
{
    if (month == 2 && day == 29 && !is_leap_year(year))
        return 28;
    return day;
}

/* Returns the date of the next birthday on or after today: this year's,
 * or next year's if this year's has already passed. */
static Date next_birthday(Date birth, Date today)
{
    Date next;

    next.year = today.year;
    next.month = birth.month;
    next.day = celebration_day(birth.month, birth.day, next.year);
    if (compare_dates(next, today) < 0) {
        next.year++;
        next.day = celebration_day(birth.month, birth.day, next.year);
    }
    return next;
}

/* Returns how many days from today until next. next must be today or later
 * and at most a year away, which is always true for the next birthday, so
 * two cases are enough: the same year, or this year's remaining days plus
 * the day of the year in the next one. */
static int days_until(Date today, Date next)
{
    if (next.year == today.year)
        return day_of_year(next) - day_of_year(today);
    int days_in_year = is_leap_year(today.year) ? 366 : 365;
    return days_in_year - day_of_year(today) + day_of_year(next);
}

/* Returns 1 and sets *out if text is a real date written YYYY-MM-DD, from
 * FIRST_YEAR on and before today; returns 0 otherwise. */
static int is_valid_birth_date(const char *text, Date today, Date *out)
{
    if (!is_valid_date(text))
        return 0;
    Date d = parse_date(text);
    if (d.year < FIRST_YEAR || compare_dates(d, today) >= 0)
        return 0;
    *out = d;
    return 1;
}

/* Prints prompt and reads a birth date into *out, asking again until it is
 * valid. Returns 1 on success, 0 at end of input. */
static int read_date(const char *prompt, Date today, Date *out)
{
    char buf[LINE_SIZE];

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;
        if (is_valid_birth_date(buf, today, out))
            return 1;
        printf("Please enter a real past date from %d-01-01, as YYYY-MM-DD.\n", FIRST_YEAR);
    }
}

/* Returns the index of the person called name (ignoring case), or -1. */
static int find_by_name(const BirthdayList *list, const char *name)
{
    for (int i = 0; i < list->count; i++)
        if (compare_ignore_case(list->people[i].name, name) == 0)
            return i;
    return -1;
}

/* qsort() comparison function: fewest days first, then by name ignoring
 * case, so "ben Kim" comes before "Zoe Park" on the same day. */
static int compare_upcoming(const void *a, const void *b)
{
    const Upcoming *x = (const Upcoming *)a;
    const Upcoming *y = (const Upcoming *)b;

    if (x->days != y->days)
        return x->days - y->days;
    return compare_ignore_case(x->person->name, y->person->name);
}

/* Fills out[] with every person's next birthday, the days until it and the
 * age they turn, then sorts it with compare_upcoming(). The key is worked
 * out once per person here, not inside the comparison function: qsort()
 * calls that many times for each element. Returns how many were filled. */
static int build_upcoming(const BirthdayList *list, Date today, Upcoming out[])
{
    for (int i = 0; i < list->count; i++) {
        const Person *p = &list->people[i];
        out[i].person = p;
        out[i].next = next_birthday(p->birth, today);
        out[i].days = days_until(today, out[i].next);
        out[i].turns = out[i].next.year - p->birth.year;
    }
    qsort(out, (size_t)list->count, sizeof out[0], compare_upcoming);
    return list->count;
}

/* Writes "today", "tomorrow" or "in N days" into out. */
static void format_when(int days, char out[WHEN_SIZE])
{
    if (days == 0)
        strcpy(out, "today");
    else if (days == 1)
        strcpy(out, "tomorrow");
    else
        snprintf(out, WHEN_SIZE, "in %d days", days);
}

/* Prints whose birthday is today or within the next REMIND_DAYS days. */
static void show_reminders(const BirthdayList *list, Date today)
{
    Upcoming up[MAX_PEOPLE];
    char when[WHEN_SIZE];
    int n = build_upcoming(list, today, up), shown = 0;

    if (n == 0) {
        printf("No birthdays saved yet.\n");
        return;
    }
    for (int i = 0; i < n && up[i].days <= REMIND_DAYS; i++) {
        if (shown++ == 0)
            printf("Coming up in the next %d days:\n", REMIND_DAYS);
        format_when(up[i].days, when);
        printf("  %-10s %s turns %d\n", when, up[i].person->name, up[i].turns);
    }
    if (shown == 0)
        printf("No birthdays in the next %d days.\n", REMIND_DAYS);
}

/* Prints everyone, soonest birthday first, with the date it is celebrated,
 * the age they turn and how far away it is. */
static void list_all(const BirthdayList *list, Date today)
{
    Upcoming up[MAX_PEOPLE];
    char when[WHEN_SIZE];
    int n = build_upcoming(list, today, up);

    if (n == 0) {
        printf("No birthdays saved yet.\n");
        return;
    }
    printf("All birthdays, soonest first:\n");
    printf("%3s  %-10s  %-25s %5s  %s\n", "#", "Next", "Name", "Turns", "When");
    for (int i = 0; i < n; i++) {
        format_when(up[i].days, when);
        printf("%3d  %04d-%02d-%02d  %-25s %5d  %s\n", i + 1, up[i].next.year, up[i].next.month,
               up[i].next.day, up[i].person->name, up[i].turns, when);
    }
}

/* Loads everyone from the file at path. A missing file means a new, empty
 * list. Lines without " | ", with a date that is not a real past date from
 * FIRST_YEAR, with an empty, too long or duplicate name, or past MAX_PEOPLE
 * are skipped and counted. Returns 0 only if the file cannot be opened. */
static int load_list(BirthdayList *list, const char *path, Date today)
{
    char line[FILE_LINE];
    char *date, *name;
    int skipped = 0;
    Date birth;
    FILE *f = fopen(path, "r");

    list->count = 0;
    if (f == NULL) {
        if (errno == ENOENT) {
            printf("No file %s yet: starting a new list.\n", path);
            return 1;
        }
        printf("Could not open %s: %s\n", path, strerror(errno));
        return 0;
    }
    while (fgets(line, sizeof line, f) != NULL) {
        if (strchr(line, '\n') == NULL && !feof(f)) {   /* too long: skip the rest */
            int c;
            while ((c = fgetc(f)) != '\n' && c != EOF)
                ;
            skipped++;
            continue;
        }
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0')
            continue;                           /* a blank line */
        if (!split_entry(line, &date, &name) || !is_valid_birth_date(date, today, &birth) ||
            name[0] == '\0' || strlen(name) >= NAME_SIZE || find_by_name(list, name) >= 0 ||
            list->count == MAX_PEOPLE) {
            skipped++;
            continue;
        }
        Person *p = &list->people[list->count++];
        strcpy(p->name, name);
        p->birth = birth;
    }
    fclose(f);                                  /* only read from: nothing to lose */

    printf("Loaded %d birthday%s from %s.\n", list->count, list->count == 1 ? "" : "s", path);
    if (skipped > 0)
        printf("(%d line%s skipped: not valid birthday lines)\n", skipped, skipped == 1 ? "" : "s");
    return 1;
}

/* Saves everyone to path the safe way, as the address book does: write
 * path.tmp, close it and check for errors, remove the old file, then rename
 * the new one. It runs after every change, so there is never anything
 * unsaved and no "changed" flag is needed. Prints nothing on success.
 * Returns 1 on success; otherwise prints why and returns 0. */
static int save_list(const BirthdayList *list, const char *path)
{
    char tmp[PATH_SIZE + 4];
    FILE *f;

    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    f = fopen(tmp, "w");
    if (f == NULL) {
        printf("Could not save: %s\n", strerror(errno));
        return 0;
    }
    for (int i = 0; i < list->count; i++) {
        const Person *p = &list->people[i];
        fprintf(f, "%04d-%02d-%02d%s%s\n", p->birth.year, p->birth.month, p->birth.day,
                SEPARATOR, p->name);
    }
    int failed = ferror(f);
    if (fclose(f) != 0 || failed ||
        (remove(path) != 0 && errno != ENOENT) || rename(tmp, path) != 0) {
        printf("Could not save: %s\n", strerror(errno));
        remove(tmp);
        return 0;
    }
    return 1;
}

/* Asks for a name and a birth date, adds the person and saves the file.
 * Returns 0 if input ended, else 1. */
static int add_birthday(BirthdayList *list, const char *path, Date today)
{
    Person p;

    if (list->count == MAX_PEOPLE) {
        printf("The list is full (%d birthdays).\n", MAX_PEOPLE);
        return 1;
    }
    for (;;) {
        if (!read_text("Name: ", p.name, NAME_SIZE))
            return 0;
        int i = find_by_name(list, p.name);
        if (i < 0)
            break;
        printf("A birthday for %s is already saved.\n", list->people[i].name);
    }
    if (!read_date("Birth date (YYYY-MM-DD): ", today, &p.birth))
        return 0;
    list->people[list->count++] = p;
    printf("Added %s.\n", p.name);
    save_list(list, path);
    return 1;
}

/* Asks for a name and deletes that person, then saves the file. The list
 * is sorted every time it is shown, so its stored order does not matter:
 * the last person simply moves into the gap. One copy, instead of shifting
 * everyone down with memmove() as the address book must to keep its order.
 * Returns 0 if input ended, else 1. */
static int delete_birthday(BirthdayList *list, const char *path)
{
    char name[NAME_SIZE];

    if (list->count == 0) {
        printf("No birthdays saved yet.\n");
        return 1;
    }
    if (!read_text("Name to delete: ", name, NAME_SIZE))
        return 0;
    int i = find_by_name(list, name);
    if (i < 0) {
        printf("No birthday saved for \"%s\".\n", name);
        return 1;
    }
    strcpy(name, list->people[i].name);         /* the stored spelling, for the message */
    list->people[i] = list->people[list->count - 1];
    list->count--;
    printf("Deleted %s.\n", name);
    save_list(list, path);
    return 1;
}
