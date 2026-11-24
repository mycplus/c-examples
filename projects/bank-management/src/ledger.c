/* ledger.c - writing entries to the ledger file and reading them back.
 *
 * Fail closed: a bank must not guess about money. The mini projects skip
 * a bad line and carry on; this loader stops at the first line that
 * breaks any rule and reports "FILE line N: REASON", so the damage can be
 * found and the file restored. Nothing from a damaged ledger is used. */
#define _CRT_SECURE_NO_WARNINGS     /* MSVC: allow the standard fopen() and strerror() */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ledger.h"

#define MAX_FIELDS 8   /* OPEN and XFER have the most fields */

/* The name of each entry type in the file, in EntryType order, and how
 * many fields a line of that type has. const tables are the only kind of
 * global this program uses. */
static const char *const type_names[] = { "OPEN", "DEP", "WDL", "XFER", "INT", "EDIT", "CLOSE" };
static const int field_counts[] = { 8, 6, 6, 8, 7, 6, 4 };
#define TYPE_COUNT ((int)(sizeof type_names / sizeof type_names[0]))

/* Writes entry as one ledger line, without the newline, into line.
 * Each type has its own fields (see the table in ledger.h and the README).
 * Returns 1 on success, 0 if the line does not fit in size characters. */
int entry_to_line(const Entry *entry, char *line, size_t size)
{
    char date[DATE_TEXT_SIZE], month[DATE_TEXT_SIZE];
    const char *type = type_names[entry->type];
    int n = -1;

    date_format(entry->date, date, sizeof date);
    switch (entry->type) {
    case ENTRY_OPEN:
        n = snprintf(line, size, "%ld|%s|%s|%d|%c|%lld|%s|%s", entry->seq, date, type,
                     entry->account, entry->account_type == ACCOUNT_SAVINGS ? 'S' : 'C',
                     entry->amount, entry->holder, entry->address);
        break;
    case ENTRY_DEPOSIT:
    case ENTRY_WITHDRAW:
        n = snprintf(line, size, "%ld|%s|%s|%d|%lld|%lld", entry->seq, date, type,
                     entry->account, entry->amount, entry->balance_after);
        break;
    case ENTRY_TRANSFER:
        n = snprintf(line, size, "%ld|%s|%s|%d|%d|%lld|%lld|%lld", entry->seq, date, type,
                     entry->account, entry->other, entry->amount,
                     entry->balance_after, entry->other_balance_after);
        break;
    case ENTRY_INTEREST:
        date_format_month(entry->month, month, sizeof month);
        n = snprintf(line, size, "%ld|%s|%s|%d|%s|%lld|%lld", entry->seq, date, type,
                     entry->account, month, entry->amount, entry->balance_after);
        break;
    case ENTRY_EDIT:
        n = snprintf(line, size, "%ld|%s|%s|%d|%s|%s", entry->seq, date, type,
                     entry->account, entry->holder, entry->address);
        break;
    case ENTRY_CLOSE:
        n = snprintf(line, size, "%ld|%s|%s|%d", entry->seq, date, type, entry->account);
        break;
    }
    return n >= 0 && (size_t)n < size;
}

/* Reads a whole number from text into *out: 1 to max_digits digits,
 * preceded by '-' only if allow_minus is 1, and nothing else. The digit
 * limit keeps the value far below the long long limit, so there is no
 * overflow to check. Returns 1 if valid, else 0. */
static int parse_number(const char *text, int allow_minus, int max_digits, long long *out)
{
    long long value = 0;
    int negative = 0, digits = 0;

    if (allow_minus && *text == '-') {
        negative = 1;
        text++;
    }
    for (; *text != '\0'; text++) {
        if (!isdigit((unsigned char)*text) || ++digits > max_digits)
            return 0;
        value = value * 10 + (*text - '0');
    }
    if (digits == 0)
        return 0;
    *out = negative ? -value : value;
    return 1;
}

/* Copies a holder name or address from the file into out. Text too long
 * for out is not cut short (that would hide the damage): out is left
 * empty instead, and bank_check() then rejects it with RULE_BAD_TEXT. */
static void copy_text(char *out, size_t size, const char *text)
{
    if (strlen(text) < size)
        strcpy(out, text);
    else
        out[0] = '\0';
}

/* Turns one ledger line (without its newline) into *entry, splitting it
 * at every '|' by writing '\0' over the separators. Checks the form of
 * the line only: the entry number and date, a known type, the right
 * number of fields for that type, and digits where numbers belong.
 * Whether the entry is allowed is bank_check()'s job.
 * Returns 1 on success; otherwise writes the reason into why and returns 0. */
int line_to_entry(char *line, Entry *entry, char *why, size_t why_size)
{
    char *field[MAX_FIELDS];
    int count = 0, type = 0;
    long long n[MAX_FIELDS] = { 0 };

    /* Split. Fields beyond MAX_FIELDS are counted but not kept. */
    field[count++] = line;
    for (char *p = strchr(line, '|'); p != NULL; p = strchr(p + 1, '|')) {
        *p = '\0';
        if (count < MAX_FIELDS)
            field[count] = p + 1;
        count++;
    }

    memset(entry, 0, sizeof *entry);
    if (!parse_number(field[0], 0, 9, &n[0])) {
        snprintf(why, why_size, "not a valid number in field 1");
        return 0;
    }
    entry->seq = (long)n[0];
    if (count < 2 || !date_parse(field[1], &entry->date)) {
        snprintf(why, why_size, "not a valid date");
        return 0;
    }
    const char *type_text = count >= 3 ? field[2] : "";
    while (type < TYPE_COUNT && strcmp(type_text, type_names[type]) != 0)
        type++;
    if (type == TYPE_COUNT) {
        snprintf(why, why_size, "unknown entry type \"%s\"", type_text);
        return 0;
    }
    entry->type = (EntryType)type;
    if (count != field_counts[type]) {
        snprintf(why, why_size, "wrong number of fields for a%s %s entry",
                 type == ENTRY_OPEN || type == ENTRY_EDIT || type == ENTRY_INTEREST ? "n" : "",
                 type_names[type]);
        return 0;
    }

    /* Which fields (1-based) are numbers, and whether they may be negative:
     * every type has its account in field 4; the rest depends on the type. */
    int numeric[MAX_FIELDS + 1] = { 0 };       /* 0: text, 1: digits, 2: digits with '-' allowed */
    numeric[4] = 1;
    switch (entry->type) {
    case ENTRY_OPEN:     numeric[6] = 1; break;
    case ENTRY_DEPOSIT:
    case ENTRY_WITHDRAW: numeric[5] = 1; numeric[6] = 2; break;
    case ENTRY_TRANSFER: numeric[5] = 1; numeric[6] = 1; numeric[7] = 2; numeric[8] = 2; break;
    case ENTRY_INTEREST: numeric[6] = 1; numeric[7] = 2; break;
    default:             break;
    }
    for (int f = 4; f <= count; f++) {
        if (numeric[f] == 0)
            continue;
        int digits = f == 4 || (entry->type == ENTRY_TRANSFER && f == 5) ? 9 : 12;
        if (!parse_number(field[f - 1], numeric[f] == 2, digits, &n[f - 1])) {
            snprintf(why, why_size, "not a valid number in field %d", f);
            return 0;
        }
    }

    entry->account = (int)n[3];
    switch (entry->type) {
    case ENTRY_OPEN:
        if (strcmp(field[4], "S") != 0 && strcmp(field[4], "C") != 0) {
            snprintf(why, why_size, "the account type must be S or C");
            return 0;
        }
        entry->account_type = field[4][0] == 'S' ? ACCOUNT_SAVINGS : ACCOUNT_CURRENT;
        entry->amount = n[5];
        entry->balance_after = n[5];            /* not stored: always the opening amount */
        copy_text(entry->holder, sizeof entry->holder, field[6]);
        copy_text(entry->address, sizeof entry->address, field[7]);
        break;
    case ENTRY_DEPOSIT:
    case ENTRY_WITHDRAW:
        entry->amount = n[4];
        entry->balance_after = n[5];
        break;
    case ENTRY_TRANSFER:
        entry->other = (int)n[4];
        entry->amount = n[5];
        entry->balance_after = n[6];
        entry->other_balance_after = n[7];
        break;
    case ENTRY_INTEREST: {
        char first_day[DATE_TEXT_SIZE + 3];
        Date d;
        snprintf(first_day, sizeof first_day, "%s-01", field[4]);
        if (strlen(field[4]) != 7 || !date_parse(first_day, &d)) {
            snprintf(why, why_size, "not a valid month in field 5");
            return 0;
        }
        entry->month = date_month_key(d);
        if (entry->month != date_month_key(entry->date)) {
            snprintf(why, why_size, "the interest month does not match the entry date");
            return 0;
        }
        entry->amount = n[5];
        entry->balance_after = n[6];
        break;
    }
    case ENTRY_EDIT:
        copy_text(entry->holder, sizeof entry->holder, field[4]);
        copy_text(entry->address, sizeof entry->address, field[5]);
        break;
    case ENTRY_CLOSE:
        break;
    }
    return 1;
}

/* Writes the reason a file entry broke rule into why, in the words of the
 * loader's error messages: "there is no account 100009",
 * "the interest should be 4.63, not 4.64", and so on. bank is the state
 * before the entry, so balances are the ones the rule was checked against. */
void rule_reason(const Bank *bank, const Entry *entry, Rule rule, char *why, size_t why_size)
{
    const Account *a = bank_find_const(bank, entry->account);
    char x[MONEY_TEXT_SIZE], y[MONEY_TEXT_SIZE], month[DATE_TEXT_SIZE];
    Cents expected;

    switch (rule) {
    case RULE_OK:
        snprintf(why, why_size, "no problem");
        break;
    case RULE_NO_ACCOUNT:
        snprintf(why, why_size, "there is no account %d", a == NULL ? entry->account : entry->other);
        break;
    case RULE_CLOSED:
        snprintf(why, why_size, "account %d is closed",
                 a != NULL && a->is_closed ? entry->account : entry->other);
        break;
    case RULE_WRONG_NUMBER:
        snprintf(why, why_size, "the next account number should be %d, not %d",
                 bank_next_number(bank), entry->account);
        break;
    case RULE_OPENING_TOO_SMALL:
        snprintf(why, why_size, "the opening deposit is below 1,000.00");
        break;
    case RULE_BAD_AMOUNT:
        snprintf(why, why_size, "the amount must be from 0.01 to 1,000,000.00");
        break;
    case RULE_INSUFFICIENT:
        snprintf(why, why_size, "this would take account %d below its limit", entry->account);
        break;
    case RULE_DAILY_LIMIT:
        snprintf(why, why_size, "this would exceed the daily withdrawal limit for account %d",
                 entry->account);
        break;
    case RULE_ABOVE_MAX:
        snprintf(why, why_size, "this would take account %d above 999,999,999.99",
                 entry->type == ENTRY_TRANSFER ? entry->other : entry->account);
        break;
    case RULE_SAME_ACCOUNT:
        snprintf(why, why_size, "a transfer must be between two different accounts");
        break;
    case RULE_NOT_SAVINGS:
        snprintf(why, why_size, "interest is only paid to savings accounts");
        break;
    case RULE_INTEREST_PAID:
        date_format_month(entry->month, month, sizeof month);
        snprintf(why, why_size, "interest for %s was already posted to account %d",
                 month, entry->account);
        break;
    case RULE_WRONG_INTEREST:
        money_format(interest_for_month(a != NULL ? a->balance : 0, SAVINGS_RATE_BP), x, sizeof x);
        money_format(entry->amount, y, sizeof y);
        snprintf(why, why_size, "the interest should be %s, not %s", x, y);
        break;
    case RULE_NOT_EMPTY:
        money_format(a != NULL ? a->balance : 0, x, sizeof x);
        snprintf(why, why_size, "account %d cannot be closed with a balance of %s",
                 entry->account, x);
        break;
    case RULE_DATE_BACKWARDS:
        snprintf(why, why_size, "the date is earlier than the previous entry");
        break;
    case RULE_BAD_TEXT:
        snprintf(why, why_size, "the holder name or address is empty, too long or contains \"|\"");
        break;
    case RULE_BALANCE_MISMATCH:
        expected = bank_balance_after(bank, entry, 0);
        if (expected == entry->balance_after) {  /* then it is the transfer target that is wrong */
            expected = bank_balance_after(bank, entry, 1);
            money_format(entry->other_balance_after, y, sizeof y);
        } else {
            money_format(entry->balance_after, y, sizeof y);
        }
        money_format(expected, x, sizeof x);
        snprintf(why, why_size, "the balance after should be %s, not %s", x, y);
        break;
    }
}

/* Reads the next line of f into buf (size LEDGER_LINE) and removes its
 * line ending. A line must end with '\n': without one, either the line
 * did not fit in buf (too long) or the file ended in the middle of a line
 * (the program stopped while writing it).
 * Returns 1 for a complete line, 0 at the end of the file, or -1 with the
 * reason in why. */
static int next_line(FILE *f, char *buf, char *why, size_t why_size)
{
    if (fgets(buf, LEDGER_LINE, f) == NULL)
        return 0;
    size_t len = strlen(buf);
    if (len == 0 || buf[len - 1] != '\n') {
        if (feof(f))
            snprintf(why, why_size, "the last line is incomplete (the program may have stopped while writing it)");
        else
            snprintf(why, why_size, "the line is too long");
        return -1;
    }
    buf[--len] = '\0';
    if (len > 0 && buf[len - 1] == '\r')
        buf[--len] = '\0';                      /* a ledger copied from Windows */
    return 1;
}

/* Loads the ledger at path into bank (which must be empty): checks the
 * header, then for every line: line_to_entry(), the entry number,
 * bank_check() and bank_apply(). The first problem stops loading.
 * Returns LOAD_OK (an empty file counts as an empty bank), LOAD_NO_FILE
 * if the file does not exist yet, LOAD_CANNOT_OPEN, or LOAD_ERROR. On
 * anything but LOAD_OK, error holds the message to show:
 * "Could not open FILE: REASON" or "FILE line N: REASON". */
LoadResult ledger_load(Bank *bank, const char *path, char *error, size_t error_size)
{
    char line[LEDGER_LINE], why[REASON_SIZE];
    long line_no = 0;
    LoadResult result = LOAD_OK;
    FILE *f;

    errno = 0;
    f = fopen(path, "r");
    if (f == NULL) {
        int err = errno;
        snprintf(error, error_size, "Could not open %s: %s", path, strerror(err));
        return err == ENOENT ? LOAD_NO_FILE : LOAD_CANNOT_OPEN;
    }

    for (;;) {
        Entry entry;
        int got = next_line(f, line, why, sizeof why);

        if (got == 0)
            break;
        line_no++;
        if (got < 0) {
            result = LOAD_ERROR;
        } else if (line_no == 1) {
            if (strcmp(line, LEDGER_HEADER) != 0) {
                snprintf(why, sizeof why, "expected the header \"%s\"", LEDGER_HEADER);
                result = LOAD_ERROR;
            }
        } else if (!line_to_entry(line, &entry, why, sizeof why)) {
            result = LOAD_ERROR;
        } else if (entry.seq != (long)bank->entry_count + 1) {
            snprintf(why, sizeof why, "expected entry number %ld, found %ld",
                     (long)bank->entry_count + 1, entry.seq);
            result = LOAD_ERROR;
        } else {
            Rule rule = bank_check(bank, &entry);
            if (rule != RULE_OK) {
                rule_reason(bank, &entry, rule, why, sizeof why);
                result = LOAD_ERROR;
            } else if (!bank_apply(bank, &entry)) {
                snprintf(why, sizeof why, "out of memory");
                result = LOAD_ERROR;
            }
        }
        if (result != LOAD_OK)
            break;
    }
    if (result == LOAD_OK && ferror(f)) {
        snprintf(why, sizeof why, "could not read the file: %s", strerror(errno));
        result = LOAD_ERROR;
        line_no++;
    }
    fclose(f);
    if (result != LOAD_OK)
        snprintf(error, error_size, "%s line %ld: %s", path, line_no, why);
    return result;
}

/* Adds entry to the end of the ledger at path, as one line. When
 * write_header is 1 (the bank has no entries yet) and the file is empty or
 * new, the header line goes first.
 *
 * The file is opened in "a" (append) mode, so existing lines are never
 * rewritten: the worst a crash can do is cut the newest line short, and
 * the loader detects that (a last line without '\n'). The whole text is
 * written with one fputs(), then fflush() and fclose() are both checked,
 * because a full disk may only be reported when the buffer is written
 * out. That is this program's version of an atomic write; a real bank
 * would use a database transaction (for example SQLite).
 * Returns 1 on success; otherwise puts the strerror() text in error and
 * returns 0. */
int ledger_append(const char *path, const Entry *entry, int write_header, char *error, size_t error_size)
{
    char line[LEDGER_LINE], text[LEDGER_LINE + sizeof LEDGER_HEADER + 2];
    FILE *f;

    if (!entry_to_line(entry, line, sizeof line)) {
        snprintf(error, error_size, "the entry is too long for one ledger line");
        return 0;
    }
    errno = 0;
    f = fopen(path, "a");
    if (f == NULL) {
        snprintf(error, error_size, "%s", strerror(errno));
        return 0;
    }
    /* Only an empty file gets the header, so a failed first write that
     * left the header behind cannot cause a second header line. */
    if (write_header && fseek(f, 0, SEEK_END) == 0 && ftell(f) == 0)
        snprintf(text, sizeof text, "%s\n%s\n", LEDGER_HEADER, line);
    else
        snprintf(text, sizeof text, "%s\n", line);

    int ok = fputs(text, f) >= 0 && fflush(f) == 0 && !ferror(f);
    int err = errno;
    if (fclose(f) != 0 && ok) {
        ok = 0;
        err = errno;
    }
    if (!ok)
        snprintf(error, error_size, "%s", strerror(err != 0 ? err : EIO));
    return ok;
}
