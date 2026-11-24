/* bank.h - accounts, ledger entries and the rules of the bank.
 *
 * Why other files go through these functions instead of changing an
 * Account directly: an account's balance is not data typed by anyone, it
 * is the sum of every ledger entry for that account. If main.c could write
 * account->balance = ... the screen could show money that no ledger line
 * explains, and the next start-up (which rebuilds every balance from the
 * file) would show a different figure. So every change goes through one
 * door: bank_check() decides whether an Entry is allowed, and
 * bank_apply() is the only function that changes balances. main.c and
 * report.c read accounts; only bank.c writes them. */
#ifndef BANK_H
#define BANK_H

#include <stddef.h>

#include "date.h"
#include "money.h"

#define SAVINGS_RATE_BP     300           /* 3.00% a year, in basis points */
#define OVERDRAFT_LIMIT     50000LL       /* current accounts may go down to -500.00 */
#define MIN_OPENING         100000LL      /* 1,000.00 */
#define DAILY_CASH_LIMIT    500000LL      /* 5,000.00 withdrawn per account per day */
#define FIRST_ACCOUNT       100001
#define LAST_ACCOUNT        999999
#define NAME_SIZE           31            /* 30 characters + '\0' */
#define ADDRESS_SIZE        51
#define MIN_NAME_LENGTH     2

typedef enum { ACCOUNT_SAVINGS, ACCOUNT_CURRENT } AccountType;

typedef struct {
    int         number;
    AccountType type;
    char        holder[NAME_SIZE];
    char        address[ADDRESS_SIZE];
    Date        opened;
    int         is_closed;
    Cents       balance;              /* rebuilt from the ledger, never read from a file */
    int         last_interest_month;  /* 202612, or 0: rebuilt from the ledger */
} Account;

typedef enum {
    ENTRY_OPEN, ENTRY_DEPOSIT, ENTRY_WITHDRAW, ENTRY_TRANSFER,
    ENTRY_INTEREST, ENTRY_EDIT, ENTRY_CLOSE
} EntryType;

/* One line of the ledger: something that happened.
 *
 * This is a "tagged struct": type is the tag, and it says which of the
 * other fields mean something (the comment on each field lists the types
 * that use it). The unused fields are simply 0 or empty. A union could
 * overlap the fields of different types and save memory, but then reading
 * the wrong member is undefined behavior that no compiler warns about. A
 * plain struct of about 150 bytes is clearer and always safe to read. */
typedef struct {
    long        seq;                  /* 1, 2, 3, ... */
    Date        date;
    EntryType   type;
    int         account;              /* the account, or the source of a transfer */
    int         other;                /* transfer target; 0 otherwise */
    AccountType account_type;         /* OPEN only */
    Cents       amount;               /* OPEN, DEPOSIT, WITHDRAW, TRANSFER, INTEREST */
    Cents       balance_after;        /* account's balance after this entry */
    Cents       other_balance_after;  /* TRANSFER only: target's balance after */
    int         month;                /* INTEREST only: 202612 */
    char        holder[NAME_SIZE];    /* OPEN and EDIT */
    char        address[ADDRESS_SIZE];/* OPEN and EDIT */
} Entry;

/* The whole bank in memory: two growing arrays. The accounts and balances
 * are only a cache; the entries (and the ledger file they came from) are
 * the truth. */
typedef struct {
    Account *accounts;  size_t account_count, account_capacity;
    Entry   *entries;   size_t entry_count,   entry_capacity;
} Bank;

/* What bank_check() decided. main.c turns each value into a message for the
 * teller; ledger.c turns it into "FILE line N: REASON". */
typedef enum {
    RULE_OK,
    RULE_NO_ACCOUNT,         /* account (or transfer target) does not exist */
    RULE_CLOSED,             /* account is closed */
    RULE_WRONG_NUMBER,       /* OPEN: number is not the next free number */
    RULE_OPENING_TOO_SMALL,  /* OPEN: below MIN_OPENING */
    RULE_BAD_AMOUNT,         /* amount outside 1..MAX_AMOUNT */
    RULE_INSUFFICIENT,       /* would go below 0.00 (savings) or -500.00 (current) */
    RULE_DAILY_LIMIT,        /* withdrawals today would exceed DAILY_CASH_LIMIT */
    RULE_ABOVE_MAX,          /* a balance would exceed MAX_BALANCE */
    RULE_SAME_ACCOUNT,       /* transfer to itself */
    RULE_NOT_SAVINGS,        /* INTEREST on a current account */
    RULE_INTEREST_PAID,      /* INTEREST already posted for that month */
    RULE_WRONG_INTEREST,     /* INTEREST amount is not interest_for_month(balance) */
    RULE_NOT_EMPTY,          /* CLOSE with a balance other than 0.00 */
    RULE_DATE_BACKWARDS,     /* entry dated before the previous entry */
    RULE_BAD_TEXT,           /* holder/address empty, too long or containing '|' */
    RULE_BALANCE_MISMATCH    /* balance_after (or other_balance_after) is not the real result */
} Rule;

void     bank_init(Bank *bank);
void     bank_free(Bank *bank);
Account *bank_find(Bank *bank, int number);                 /* NULL if none */
const Account *bank_find_const(const Bank *bank, int number);
int      bank_next_number(const Bank *bank);                /* FIRST_ACCOUNT if empty */
Cents    bank_available(const Account *account);            /* balance, plus 500.00 for current */
Cents    bank_withdrawn_on(const Bank *bank, int number, Date day);   /* sum of WITHDRAW entries */
Cents    bank_balance_after(const Bank *bank, const Entry *entry, int for_other);
Cents    bank_money_held(const Bank *bank);                 /* sum of all balances */
int      bank_text_ok(const char *text, size_t min_len, size_t size);  /* the RULE_BAD_TEXT test */
Rule     bank_check(const Bank *bank, const Entry *entry);  /* also checks balance_after fields */
int      bank_apply(Bank *bank, const Entry *entry);        /* 0 only if out of memory */
int      bank_prepare(const Bank *bank, Entry *entry);      /* fills seq and balance_after fields */

#endif
