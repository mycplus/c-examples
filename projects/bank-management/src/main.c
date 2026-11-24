/* Bank Management System - a teller's console program for a small bank.
 *
 * Accounts (savings and current), deposits, withdrawals, transfers and
 * monthly interest, kept in one append-only ledger file that is checked
 * line by line every time the program starts.
 *
 * Build every .c file in src together, for example:
 *     gcc -std=c11 -Wall -Wextra -o bank_management src/main.c src/bank.c
 *         src/date.c src/input.c src/ledger.c src/money.c src/report.c
 *     cl /W4 /Fe:bank_management.exe src\main.c src\bank.c src\date.c
 *         src\input.c src\ledger.c src\money.c src\report.c
 * or use CMake, which also builds and runs the tests (see the README).
 * Run:    bank_management [--file NAME] [--today YYYY-MM-DD] [--verify]
 *
 * This file is the user interface: the command line, the start-up checks
 * and one handler per menu option. The rules live in bank.c, the file in
 * ledger.c and the tables in report.c. Every handler that changes data
 * takes the same six steps:
 *     read -> build an Entry -> bank_prepare -> bank_check -> ledger_append -> bank_apply
 * and returns 0 when input ended, 1 otherwise. */
#define _CRT_SECURE_NO_WARNINGS     /* MSVC: allow the standard strcpy() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bank.h"
#include "date.h"
#include "input.h"
#include "ledger.h"
#include "money.h"
#include "report.h"

#define DEFAULT_FILE  "bank.txt"
#define ERROR_SIZE    512
#define PROMPT_SIZE   96

/* What every handler needs: the bank, its ledger file and today's date.
 * Passed by pointer instead of being global variables. */
typedef struct {
    Bank        bank;
    const char *path;
    Date        today;
} Teller;

static int  parse_args(int argc, char *argv[], const char **path, Date *today, int *verify);
static int  verify_ledger(const char *path);
static int  run_menu(Teller *t);
static int  do_open(Teller *t);
static int  do_deposit(Teller *t);
static int  do_withdraw(Teller *t);
static int  do_transfer(Teller *t);
static int  do_statement(Teller *t);
static int  do_list(Teller *t);
static int  do_search(Teller *t);
static int  do_daily_report(Teller *t);
static int  do_interest(Teller *t);
static int  do_update(Teller *t);
static int  do_close(Teller *t);
static int  read_account(Teller *t, const char *prompt, int allow_closed, const Account **out);
static int  read_field(const char *prompt, char *out, size_t size, size_t min_len, const char *keep);
static Entry new_entry(const Teller *t, EntryType type, int account);
static Rule prepare_and_check(Teller *t, Entry *entry);
static void explain(const Teller *t, const Entry *entry, Rule rule);
static int  save_entry(Teller *t, const Entry *entry, char *error, size_t error_size);

int main(int argc, char *argv[])
{
    Teller t;
    char error[ERROR_SIZE], date[DATE_TEXT_SIZE], last[DATE_TEXT_SIZE];
    int verify = 0;

    t.path = DEFAULT_FILE;
    t.today = date_today();
    if (!parse_args(argc, argv, &t.path, &t.today, &verify)) {
        fprintf(stderr, "Usage: bank_management [--file NAME] [--today YYYY-MM-DD] [--verify]\n");
        return 1;
    }
    if (verify)
        return verify_ledger(t.path);

    date_format(t.today, date, sizeof date);
    printf("Bank Management System\n======================\nToday is %s.\n", date);

    bank_init(&t.bank);
    switch (ledger_load(&t.bank, t.path, error, sizeof error)) {
    case LOAD_NO_FILE:
        printf("No ledger %s yet: starting a new bank.\n", t.path);
        break;
    case LOAD_CANNOT_OPEN:
        fprintf(stderr, "%s\n", error);
        bank_free(&t.bank);
        return 2;
    case LOAD_ERROR:
        fprintf(stderr, "%s\nThe ledger was not changed. Fix or restore it, then try again.\n", error);
        bank_free(&t.bank);
        return 2;
    case LOAD_OK: {
        /* A clock set in the past must not let anyone post backdated entries. */
        if (t.bank.entry_count > 0 &&
            date_compare(t.today, t.bank.entries[t.bank.entry_count - 1].date) < 0) {
            date_format(t.bank.entries[t.bank.entry_count - 1].date, last, sizeof last);
            fprintf(stderr, "Today (%s) is earlier than the last ledger entry (%s). "
                            "Check the date and try again.\n", date, last);
            bank_free(&t.bank);
            return 2;
        }
        printf("Loaded %d account(s) and %d ledger %s from %s.\n", (int)t.bank.account_count,
               (int)t.bank.entry_count, t.bank.entry_count == 1 ? "entry" : "entries", t.path);
        break;
    }
    }

    if (!run_menu(&t))
        printf("\n\nGoodbye!\n");               /* input ended at a prompt */
    else
        printf("\nGoodbye!\n");
    bank_free(&t.bank);
    return 0;
}

/* Reads the command line: --file NAME, --today YYYY-MM-DD and --verify,
 * in any order. Returns 1 if every argument is understood (and the date
 * is real), otherwise 0. */
static int parse_args(int argc, char *argv[], const char **path, Date *today, int *verify)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--verify") == 0) {
            *verify = 1;
        } else if (strcmp(argv[i], "--file") == 0 && i + 1 < argc && argv[i + 1][0] != '\0') {
            *path = argv[++i];
        } else if (strcmp(argv[i], "--today") == 0 && i + 1 < argc) {
            if (!date_parse(argv[++i], today))
                return 0;
        } else {
            return 0;
        }
    }
    return 1;
}

/* --verify: loads and checks the whole ledger without showing the menu,
 * then prints a one-line summary. Useful after a crash, a restore from a
 * backup or a hand edit. Returns the program's exit code: 0 if the ledger
 * is sound, 2 if it is damaged or cannot be opened. */
static int verify_ledger(const char *path)
{
    Bank bank;
    char error[ERROR_SIZE], last[DATE_TEXT_SIZE], held[MONEY_TEXT_SIZE];

    bank_init(&bank);
    if (ledger_load(&bank, path, error, sizeof error) != LOAD_OK) {
        fprintf(stderr, "%s\n", error);
        bank_free(&bank);
        return 2;
    }
    if (bank.entry_count == 0)
        strcpy(last, "none");
    else
        date_format(bank.entries[bank.entry_count - 1].date, last, sizeof last);
    money_format(bank_money_held(&bank), held, sizeof held);
    printf("OK: %d account(s), %d ledger %s, last entry %s. Money held: %s.\n",
           (int)bank.account_count, (int)bank.entry_count,
           bank.entry_count == 1 ? "entry" : "entries", last, held);
    bank_free(&bank);
    return 0;
}

/* Shows the menu and runs the chosen option until the teller chooses 0.
 * This one loop is the only place the program goes "back to the menu":
 * every handler returns here when it is done.
 * Returns 1 after option 0, or 0 if input ended. */
static int run_menu(Teller *t)
{
    /* One handler per option, indexed by the option number. */
    static int (*const handlers[])(Teller *) = {
        NULL, do_open, do_deposit, do_withdraw, do_transfer, do_statement, do_list,
        do_search, do_daily_report, do_interest, do_update, do_close
    };
    int choice = 0;

    for (;;) {
        printf("\n 1. Open an account\n 2. Deposit\n 3. Withdraw\n 4. Transfer\n"
               " 5. Account statement\n 6. List all accounts\n 7. Search by name\n"
               " 8. Daily transaction report\n 9. Post monthly interest\n"
               "10. Update holder details\n11. Close an account\n 0. Quit\n");
        if (!read_int("Choose an option (0-11): ", 0, 11, &choice))
            return 0;
        if (choice == 0)
            return 1;
        if (!handlers[choice](t))
            return 0;
    }
}

/* Option 1: asks for the holder's name and address, the account type and
 * an opening deposit of at least 1,000.00, then opens the account with the
 * next free number. Returns 0 if input ended, otherwise 1. */
static int do_open(Teller *t)
{
    Entry e = new_entry(t, ENTRY_OPEN, bank_next_number(&t->bank));
    char error[ERROR_SIZE], amount[MONEY_TEXT_SIZE];
    int type = 1;

    if (e.account > LAST_ACCOUNT) {
        printf("No account numbers are left.\n");
        return 1;
    }
    if (!read_field("Holder name: ", e.holder, NAME_SIZE, MIN_NAME_LENGTH, NULL) ||
        !read_field("Address: ", e.address, ADDRESS_SIZE, 1, NULL) ||
        !read_int("Account type (1 = Savings, 2 = Current): ", 1, 2, &type))
        return 0;
    e.account_type = type == 1 ? ACCOUNT_SAVINGS : ACCOUNT_CURRENT;
    for (;;) {
        if (!read_money("Opening deposit: ", &e.amount))
            return 0;
        if (e.amount >= MIN_OPENING)
            break;
        printf("The opening deposit must be at least 1,000.00.\n");
    }

    Rule rule = prepare_and_check(t, &e);
    if (rule != RULE_OK) {
        explain(t, &e, rule);
        return 1;
    }
    if (!save_entry(t, &e, error, sizeof error))
        return 1;
    money_format(e.amount, amount, sizeof amount);
    printf("Opened %s account %d for %s with %s.\n", type == 1 ? "savings" : "current",
           e.account, e.holder, amount);
    return 1;
}

/* Option 2: pays an amount of cash into an open account.
 * Returns 0 if input ended, otherwise 1. */
static int do_deposit(Teller *t)
{
    const Account *a;
    Entry e;
    char error[ERROR_SIZE], amount[MONEY_TEXT_SIZE], balance[MONEY_TEXT_SIZE];

    if (!read_account(t, "Account number: ", 0, &a))
        return 0;
    if (a == NULL)
        return 1;
    e = new_entry(t, ENTRY_DEPOSIT, a->number);
    if (!read_money("Amount: ", &e.amount))
        return 0;
    Rule rule = prepare_and_check(t, &e);
    if (rule != RULE_OK) {
        explain(t, &e, rule);
        return 1;
    }
    if (!save_entry(t, &e, error, sizeof error))
        return 1;
    money_format(e.amount, amount, sizeof amount);
    money_format(e.balance_after, balance, sizeof balance);
    printf("Deposited %s to %d. New balance: %s.\n", amount, e.account, balance);
    return 1;
}

/* Option 3: pays cash out of an open account, within its available money
 * (the overdraft included) and the daily cash limit.
 * Returns 0 if input ended, otherwise 1. */
static int do_withdraw(Teller *t)
{
    const Account *a;
    Entry e;
    char error[ERROR_SIZE], amount[MONEY_TEXT_SIZE], balance[MONEY_TEXT_SIZE];

    if (!read_account(t, "Account number: ", 0, &a))
        return 0;
    if (a == NULL)
        return 1;
    e = new_entry(t, ENTRY_WITHDRAW, a->number);
    if (!read_money("Amount: ", &e.amount))
        return 0;
    Rule rule = prepare_and_check(t, &e);
    if (rule != RULE_OK) {
        explain(t, &e, rule);
        return 1;
    }
    if (!save_entry(t, &e, error, sizeof error))
        return 1;
    money_format(e.amount, amount, sizeof amount);
    money_format(e.balance_after, balance, sizeof balance);
    printf("Withdrew %s from %d. New balance: %s.\n", amount, e.account, balance);
    return 1;
}

/* Option 4: moves money between two different open accounts. It is one
 * ledger line, so it can never be half done: there is no moment when the
 * money has left one account but not reached the other. Transfers do not
 * count toward the daily cash limit. Returns 0 if input ended, otherwise 1. */
static int do_transfer(Teller *t)
{
    const Account *from, *to;
    Entry e;
    int to_number = 0;
    char error[ERROR_SIZE], amount[MONEY_TEXT_SIZE], from_text[MONEY_TEXT_SIZE], to_text[MONEY_TEXT_SIZE];

    if (!read_account(t, "From account: ", 0, &from))
        return 0;
    if (from == NULL)
        return 1;
    if (!read_int("To account: ", FIRST_ACCOUNT, LAST_ACCOUNT, &to_number))
        return 0;
    if (to_number == from->number) {
        printf("Choose two different accounts.\n");
        return 1;
    }
    to = bank_find_const(&t->bank, to_number);
    if (to == NULL || to->is_closed) {
        printf(to == NULL ? "There is no account %d.\n" : "Account %d is closed.\n", to_number);
        return 1;
    }
    e = new_entry(t, ENTRY_TRANSFER, from->number);
    e.other = to->number;
    if (!read_money("Amount: ", &e.amount))
        return 0;
    Rule rule = prepare_and_check(t, &e);
    if (rule != RULE_OK) {
        explain(t, &e, rule);
        return 1;
    }
    if (!save_entry(t, &e, error, sizeof error))
        return 1;
    money_format(e.amount, amount, sizeof amount);
    money_format(e.balance_after, from_text, sizeof from_text);
    money_format(e.other_balance_after, to_text, sizeof to_text);
    printf("Transferred %s from %d to %d.\nNew balances: %d %s, %d %s.\n",
           amount, e.account, e.other, e.account, from_text, e.other, to_text);
    return 1;
}

/* Option 5: prints the statement of any account, open or closed.
 * Returns 0 if input ended, otherwise 1. */
static int do_statement(Teller *t)
{
    const Account *a;

    if (!read_account(t, "Account number: ", 1, &a))
        return 0;
    if (a != NULL)
        print_statement(&t->bank, a);
    return 1;
}

/* Option 6: lists every account. Always returns 1: it asks nothing. */
static int do_list(Teller *t)
{
    print_accounts(&t->bank);
    return 1;
}

/* Option 7: lists the accounts whose holder name contains the typed text,
 * ignoring case. Returns 0 if input ended, otherwise 1. */
static int do_search(Teller *t)
{
    char text[NAME_SIZE];

    if (!read_text("Search for: ", text, sizeof text))
        return 0;
    print_search(&t->bank, text);
    return 1;
}

/* Option 8: prints every transaction on one date (Enter for today).
 * Returns 0 if input ended, otherwise 1. */
static int do_daily_report(Teller *t)
{
    Date day;

    if (!read_date_or_today("Date (YYYY-MM-DD, or Enter for today): ", t->today, &day))
        return 0;
    print_daily_report(&t->bank, day);
    return 1;
}

/* Option 9: posts this month's interest to every open savings account
 * with a positive balance that has not had it yet, in account order.
 *
 * Idempotent: the ledger remembers which accounts were paid for this
 * month, so running the option twice pays nothing the second time. And if
 * a write fails after 20 of 50 accounts, running it again pays only the
 * 30 that are missing. Always returns 1: it asks nothing. */
static int do_interest(Teller *t)
{
    int month = date_month_key(t->today), due = 0, already = 0, posted = 0;
    Cents total = 0;
    char month_text[DATE_TEXT_SIZE], error[ERROR_SIZE], total_text[MONEY_TEXT_SIZE];

    date_format_month(month, month_text, sizeof month_text);
    for (size_t i = 0; i < t->bank.account_count; i++) {
        const Account *a = &t->bank.accounts[i];
        if (a->is_closed || a->type != ACCOUNT_SAVINGS)
            continue;
        if (a->last_interest_month >= month)
            already++;
        else if (interest_for_month(a->balance, SAVINGS_RATE_BP) > 0)
            due++;
    }
    if (due == 0) {
        if (already > 0)
            printf("Interest for %s was already posted. Nothing was changed.\n", month_text);
        else
            printf("No savings account is due interest for %s.\n", month_text);
        return 1;
    }

    /* Index loop, not a pointer: save_entry() may grow (and move) the arrays. */
    for (size_t i = 0; i < t->bank.account_count; i++) {
        const Account *a = &t->bank.accounts[i];
        if (a->is_closed || a->type != ACCOUNT_SAVINGS || a->last_interest_month >= month)
            continue;
        Entry e = new_entry(t, ENTRY_INTEREST, a->number);
        e.month = month;
        e.amount = interest_for_month(a->balance, SAVINGS_RATE_BP);
        if (e.amount <= 0)
            continue;
        if (prepare_and_check(t, &e) != RULE_OK)
            continue;                           /* cannot happen: the same tests as above */
        if (!save_entry(t, &e, error, sizeof error)) {
            printf("Could not save: %s. Interest was posted to %d of %d account(s); "
                   "run this option again to finish.\n", error, posted, due);
            return 1;
        }
        posted++;
        total += e.amount;
    }
    money_format(total, total_text, sizeof total_text);
    printf("Posted interest for %s to %d account(s): %s in total.\n", month_text, posted, total_text);
    return 1;
}

/* Option 10: changes the holder name and/or address of an open account.
 * Pressing Enter keeps the value shown in brackets. The ledger keeps both
 * the old values (in earlier entries) and the new ones (in this EDIT).
 * Returns 0 if input ended, otherwise 1. */
static int do_update(Teller *t)
{
    const Account *a;
    Entry e;
    char prompt[PROMPT_SIZE], error[ERROR_SIZE];

    if (!read_account(t, "Account number: ", 0, &a))
        return 0;
    if (a == NULL)
        return 1;
    e = new_entry(t, ENTRY_EDIT, a->number);
    snprintf(prompt, sizeof prompt, "Holder name [%s]: ", a->holder);
    if (!read_field(prompt, e.holder, NAME_SIZE, MIN_NAME_LENGTH, a->holder))
        return 0;
    snprintf(prompt, sizeof prompt, "Address [%s]: ", a->address);
    if (!read_field(prompt, e.address, ADDRESS_SIZE, 1, a->address))
        return 0;
    if (strcmp(e.holder, a->holder) == 0 && strcmp(e.address, a->address) == 0) {
        printf("Nothing was changed.\n");
        return 1;
    }
    Rule rule = prepare_and_check(t, &e);
    if (rule != RULE_OK) {
        explain(t, &e, rule);
        return 1;
    }
    if (!save_entry(t, &e, error, sizeof error))
        return 1;
    printf("Updated account %d.\n", e.account);
    return 1;
}

/* Option 11: closes an account whose balance is exactly 0.00, after a y/n
 * confirmation. A closed account keeps its number and its history, but
 * accepts no new transactions. Returns 0 if input ended, otherwise 1. */
static int do_close(Teller *t)
{
    const Account *a;
    Entry e;
    int number = 0, yes = 0;
    char prompt[PROMPT_SIZE], error[ERROR_SIZE], balance[MONEY_TEXT_SIZE];

    if (!read_int("Account number: ", FIRST_ACCOUNT, LAST_ACCOUNT, &number))
        return 0;
    a = bank_find_const(&t->bank, number);
    if (a == NULL) {
        printf("There is no account %d.\n", number);
        return 1;
    }
    if (a->is_closed) {
        printf("Account %d is already closed.\n", number);
        return 1;
    }
    if (a->balance > 0) {
        money_format(a->balance, balance, sizeof balance);
        printf("Account %d still holds %s. Withdraw or transfer it first.\n", number, balance);
        return 1;
    }
    if (a->balance < 0) {
        money_format(-a->balance, balance, sizeof balance);
        printf("Account %d is overdrawn by %s. Deposit it first.\n", number, balance);
        return 1;
    }
    snprintf(prompt, sizeof prompt, "Close account %d for %s? (y/n): ", number, a->holder);
    if (!read_yes_no(prompt, &yes))
        return 0;
    if (!yes) {
        printf("Nothing was changed.\n");
        return 1;
    }
    e = new_entry(t, ENTRY_CLOSE, number);
    Rule rule = prepare_and_check(t, &e);
    if (rule != RULE_OK) {
        explain(t, &e, rule);
        return 1;
    }
    if (!save_entry(t, &e, error, sizeof error))
        return 1;
    printf("Closed account %d.\n", number);
    return 1;
}

/* Asks for an account number (100001 to 999999) and sets *out to that
 * account. If there is no such account, or it is closed and allow_closed
 * is 0, prints why and sets *out to NULL, so the caller stops before
 * asking anything else. Returns 0 if input ended, otherwise 1. */
static int read_account(Teller *t, const char *prompt, int allow_closed, const Account **out)
{
    int number = 0;

    *out = NULL;
    if (!read_int(prompt, FIRST_ACCOUNT, LAST_ACCOUNT, &number))
        return 0;
    const Account *a = bank_find_const(&t->bank, number);
    if (a == NULL)
        printf("There is no account %d.\n", number);
    else if (a->is_closed && !allow_closed)
        printf("Account %d is closed.\n", number);
    else
        *out = a;
    return 1;
}

/* Prints prompt and reads a holder name or address into out (size bytes),
 * without spaces at either end. It must be min_len to size - 1 characters
 * and must not contain '|', the ledger's field separator; anything else
 * asks again. If keep is not NULL, an empty answer keeps that old value
 * (read_text() cannot be used for that, because it rejects empty input).
 * Returns 1 on success, 0 at end of input. */
static int read_field(const char *prompt, char *out, size_t size, size_t min_len, const char *keep)
{
    char buf[LINE_SIZE];

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;
        trim(buf);
        if (buf[0] == '\0' && keep != NULL) {
            strcpy(out, keep);
            return 1;
        }
        if (strchr(buf, '|') != NULL) {
            printf("Please do not use the | character.\n");
        } else if (strlen(buf) < min_len || strlen(buf) > size - 1) {
            printf("Please enter %d to %d characters.\n", (int)min_len, (int)(size - 1));
        } else {
            strcpy(out, buf);
            return 1;
        }
    }
}

/* Returns a new entry of the given type for account, dated today, with
 * every other field 0 or empty. The caller fills in the rest. */
static Entry new_entry(const Teller *t, EntryType type, int account)
{
    Entry e;

    memset(&e, 0, sizeof e);
    e.type = type;
    e.date = t->today;
    e.account = account;
    return e;
}

/* Fills in the entry number and balances with bank_prepare(), then asks
 * bank_check() whether the entry is allowed. Returns the Rule. */
static Rule prepare_and_check(Teller *t, Entry *entry)
{
    bank_prepare(&t->bank, entry);
    return bank_check(&t->bank, entry);
}

/* Prints the teller's message for a rule that refused entry. The usual
 * ones have their own wording; anything else (which the menu's own checks
 * should prevent) falls back to the loader's reason. */
static void explain(const Teller *t, const Entry *entry, Rule rule)
{
    const Account *a = bank_find_const(&t->bank, entry->account);
    char x[MONEY_TEXT_SIZE], why[REASON_SIZE];

    switch (rule) {
    case RULE_INSUFFICIENT:
        money_format(bank_available(a), x, sizeof x);
        if (a->type == ACCOUNT_CURRENT)
            printf("Insufficient funds: %d has %s available, including the 500.00 overdraft.\n",
                   a->number, x);
        else
            printf("Insufficient funds: %d has %s available.\n", a->number, x);
        break;
    case RULE_DAILY_LIMIT:
        money_format(DAILY_CASH_LIMIT - bank_withdrawn_on(&t->bank, a->number, entry->date), x, sizeof x);
        printf("Daily withdrawal limit: %d can withdraw %s more today.\n", a->number, x);
        break;
    case RULE_ABOVE_MAX:
        printf("That would take account %d above 999,999,999.99.\n",
               entry->type == ENTRY_TRANSFER ? entry->other : entry->account);
        break;
    case RULE_OPENING_TOO_SMALL:
        printf("The opening deposit must be at least 1,000.00.\n");
        break;
    default:
        rule_reason(&t->bank, entry, rule, why, sizeof why);
        printf("Not allowed: %s.\n", why);
        break;
    }
}

/* Write first, then change memory: appends entry to the ledger file and,
 * only if that worked, applies it to the bank in memory.
 *
 * The opposite order is dangerous: if memory changed first and the write
 * then failed, the screen would show money that the file does not have,
 * and the next start-up would silently "lose" it.
 * Returns 1 on success. If the write fails, prints
 * "Could not save: REASON. Nothing was changed.", leaves error set and
 * returns 0. If memory runs out after the write, memory and file no longer
 * agree, so the program stops; the next start reloads the file, which is
 * correct. */
static int save_entry(Teller *t, const Entry *entry, char *error, size_t error_size)
{
    if (!ledger_append(t->path, entry, t->bank.entry_count == 0, error, error_size)) {
        if (entry->type != ENTRY_INTEREST)
            printf("Could not save: %s. Nothing was changed.\n", error);
        return 0;
    }
    if (!bank_apply(&t->bank, entry)) {
        fprintf(stderr, "Out of memory. The entry was saved; run the program again to reload the ledger.\n");
        bank_free(&t->bank);
        exit(EXIT_FAILURE);
    }
    return 1;
}
