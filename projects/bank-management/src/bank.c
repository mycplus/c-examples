/* bank.c - the rules of the bank, and the only code that changes balances.
 *
 * Event sourcing in miniature: the ledger stores what happened (opened,
 * deposited, withdrew...), never the current state. bank_apply() replays
 * one entry, and loading is "read every line, check it, apply it". So a
 * balance is always exactly the sum of the ledger.
 * The trade-off: start-up time grows with the ledger, because every entry
 * since the bank opened is replayed. Real systems store periodic snapshots
 * (say, every balance at the end of each month) and replay only the
 * entries after the latest snapshot. */
#define _CRT_SECURE_NO_WARNINGS     /* MSVC: allow the standard strcpy() */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "bank.h"

/* Sets up an empty bank: no accounts, no entries, nothing allocated yet. */
void bank_init(Bank *bank)
{
    bank->accounts = NULL;
    bank->account_count = 0;
    bank->account_capacity = 0;
    bank->entries = NULL;
    bank->entry_count = 0;
    bank->entry_capacity = 0;
}

/* Frees both arrays and leaves the bank empty, ready to be used again.
 * free(NULL) does nothing, so this is safe on a bank that never grew. */
void bank_free(Bank *bank)
{
    free(bank->accounts);
    free(bank->entries);
    bank_init(bank);
}

/* Returns the account with this number, or NULL if there is none.
 * Numbers are given out in order from FIRST_ACCOUNT and never reused
 * (bank_check enforces it), so account 100005 is always at index 4: no
 * search is needed. The number is still compared, as a cheap guard. */
Account *bank_find(Bank *bank, int number)
{
    if (number < FIRST_ACCOUNT)
        return NULL;
    size_t index = (size_t)(number - FIRST_ACCOUNT);
    if (index >= bank->account_count || bank->accounts[index].number != number)
        return NULL;
    return &bank->accounts[index];
}

/* The same as bank_find(), for code that only reads (reports, the rules).
 * C has no overloading, so a const version needs its own name. */
const Account *bank_find_const(const Bank *bank, int number)
{
    if (number < FIRST_ACCOUNT)
        return NULL;
    size_t index = (size_t)(number - FIRST_ACCOUNT);
    if (index >= bank->account_count || bank->accounts[index].number != number)
        return NULL;
    return &bank->accounts[index];
}

/* Returns the number the next account will get: the highest number so far
 * plus 1, or FIRST_ACCOUNT for an empty bank. Closed accounts keep their
 * numbers, so a number is never given out twice. The result may be above
 * LAST_ACCOUNT, which means no numbers are left. */
int bank_next_number(const Bank *bank)
{
    if (bank->account_count == 0)
        return FIRST_ACCOUNT;
    return bank->accounts[bank->account_count - 1].number + 1;
}

/* Returns how much can be taken out of the account: the balance, plus the
 * 500.00 overdraft for a current account. A current account at -399.50
 * has 100.50 available. */
Cents bank_available(const Account *account)
{
    if (account->type == ACCOUNT_CURRENT)
        return account->balance + OVERDRAFT_LIMIT;
    return account->balance;
}

/* Returns the total withdrawn in cash from account number on day.
 * This is worked out from the ledger, not stored: there is no "withdrawn
 * today" counter to forget to reset at midnight. Entries are in date
 * order, so the loop walks back from the newest and stops at the first
 * entry before day. Transfers are not cash, so they do not count. */
Cents bank_withdrawn_on(const Bank *bank, int number, Date day)
{
    Cents total = 0;

    for (size_t i = bank->entry_count; i > 0; i--) {
        const Entry *e = &bank->entries[i - 1];
        if (date_compare(e->date, day) < 0)
            break;
        if (e->type == ENTRY_WITHDRAW && e->account == number && date_compare(e->date, day) == 0)
            total += e->amount;
    }
    return total;
}

/* Returns what the balance of the entry's account (or, when for_other is
 * 1, of the transfer target) will be after the entry is applied. Used both
 * to fill in a new entry and to check one read from the file. Returns 0
 * if the account does not exist; bank_check reports that separately. */
Cents bank_balance_after(const Bank *bank, const Entry *entry, int for_other)
{
    const Account *a = bank_find_const(bank, for_other ? entry->other : entry->account);

    if (entry->type == ENTRY_OPEN)
        return entry->amount;
    if (a == NULL)
        return 0;
    switch (entry->type) {
    case ENTRY_DEPOSIT:
    case ENTRY_INTEREST:
        return a->balance + entry->amount;
    case ENTRY_WITHDRAW:
        return a->balance - entry->amount;
    case ENTRY_TRANSFER:
        return for_other ? a->balance + entry->amount : a->balance - entry->amount;
    default:                                    /* EDIT and CLOSE move no money */
        return a->balance;
    }
}

/* Returns the money held by the bank: the sum of every balance. Overdrawn
 * current accounts count as negative. */
Cents bank_money_held(const Bank *bank)
{
    Cents total = 0;

    for (size_t i = 0; i < bank->account_count; i++)
        total += bank->accounts[i].balance;
    return total;
}

/* Returns 1 if text can be stored as a holder name or address: from
 * min_len to size - 1 characters, no '|' (the ledger's field separator)
 * and no control characters (they would be printed to the terminal as
 * they are). Forbidding the character is simpler than quoting it. */
int bank_text_ok(const char *text, size_t min_len, size_t size)
{
    size_t len = strlen(text);

    if (len < min_len || len > size - 1 || strchr(text, '|') != NULL)
        return 0;
    for (const char *p = text; *p != '\0'; p++)
        if (iscntrl((unsigned char)*p))
            return 0;
    return 1;
}

/* Returns 1 if amount is a valid single transaction: 0.01 to 1,000,000.00. */
static int amount_ok(Cents amount)
{
    return amount >= 1 && amount <= MAX_AMOUNT;
}

/* Decides whether entry may be added to the bank, and returns RULE_OK or
 * the first rule it breaks. The checks run in this order:
 *   1. the date is not earlier than the previous entry's date;
 *   2. the rules for this type of entry (listed in the README);
 *   3. balance_after (and other_balance_after) equal what applying the
 *      entry would really produce.
 *
 * One set of rules for two jobs: the menu calls this before writing an
 * entry, and the loader calls it for every line it replays. So a ledger
 * edited by hand cannot sneak in an overdraft the menu would refuse.
 * Step 3 can only fail for entries read from a file, because the menu
 * fills those fields with bank_prepare(). */
Rule bank_check(const Bank *bank, const Entry *entry)
{
    const Account *a = NULL, *b = NULL;

    if (bank->entry_count > 0 &&
        date_compare(entry->date, bank->entries[bank->entry_count - 1].date) < 0)
        return RULE_DATE_BACKWARDS;

    if (entry->type == ENTRY_OPEN) {
        int next = bank_next_number(bank);
        if (entry->account != next || next > LAST_ACCOUNT)
            return RULE_WRONG_NUMBER;
        if (!amount_ok(entry->amount))
            return RULE_BAD_AMOUNT;
        if (entry->amount < MIN_OPENING)
            return RULE_OPENING_TOO_SMALL;
        if (!bank_text_ok(entry->holder, MIN_NAME_LENGTH, NAME_SIZE) ||
            !bank_text_ok(entry->address, 1, ADDRESS_SIZE))
            return RULE_BAD_TEXT;
        return RULE_OK;                         /* the balance after opening is the amount */
    }

    /* Every other entry needs an existing, open account. */
    a = bank_find_const(bank, entry->account);
    if (a == NULL)
        return RULE_NO_ACCOUNT;
    if (a->is_closed)
        return RULE_CLOSED;

    switch (entry->type) {
    case ENTRY_DEPOSIT:
        if (!amount_ok(entry->amount))
            return RULE_BAD_AMOUNT;
        if (a->balance + entry->amount > MAX_BALANCE)
            return RULE_ABOVE_MAX;
        break;
    case ENTRY_WITHDRAW:
        if (!amount_ok(entry->amount))
            return RULE_BAD_AMOUNT;
        if (entry->amount > bank_available(a))
            return RULE_INSUFFICIENT;
        if (bank_withdrawn_on(bank, a->number, entry->date) + entry->amount > DAILY_CASH_LIMIT)
            return RULE_DAILY_LIMIT;
        break;
    case ENTRY_TRANSFER:
        if (entry->other == entry->account)
            return RULE_SAME_ACCOUNT;
        b = bank_find_const(bank, entry->other);
        if (b == NULL)
            return RULE_NO_ACCOUNT;
        if (b->is_closed)
            return RULE_CLOSED;
        if (!amount_ok(entry->amount))
            return RULE_BAD_AMOUNT;
        if (entry->amount > bank_available(a))
            return RULE_INSUFFICIENT;
        if (b->balance + entry->amount > MAX_BALANCE)
            return RULE_ABOVE_MAX;
        break;
    case ENTRY_INTEREST:
        if (a->type != ACCOUNT_SAVINGS)
            return RULE_NOT_SAVINGS;
        if (a->last_interest_month >= entry->month)
            return RULE_INTEREST_PAID;
        if (entry->amount != interest_for_month(a->balance, SAVINGS_RATE_BP))
            return RULE_WRONG_INTEREST;
        if (!amount_ok(entry->amount))
            return RULE_BAD_AMOUNT;             /* an interest of 0.00 is never posted */
        if (a->balance + entry->amount > MAX_BALANCE)
            return RULE_ABOVE_MAX;
        break;
    case ENTRY_EDIT:
        if (!bank_text_ok(entry->holder, MIN_NAME_LENGTH, NAME_SIZE) ||
            !bank_text_ok(entry->address, 1, ADDRESS_SIZE))
            return RULE_BAD_TEXT;
        break;
    case ENTRY_CLOSE:
        if (a->balance != 0)
            return RULE_NOT_EMPTY;
        break;
    default:
        break;
    }

    /* Step 3: the balances written in the entry must be the real ones. */
    if (entry->type != ENTRY_EDIT && entry->type != ENTRY_CLOSE &&
        entry->balance_after != bank_balance_after(bank, entry, 0))
        return RULE_BALANCE_MISMATCH;
    if (entry->type == ENTRY_TRANSFER &&
        entry->other_balance_after != bank_balance_after(bank, entry, 1))
        return RULE_BALANCE_MISMATCH;
    return RULE_OK;
}

/* Makes room for one more element in an array that grows by doubling.
 * *items points to the array, *capacity is its size in elements.
 * realloc() goes into a temporary pointer first: if it fails it returns
 * NULL but the old block is still allocated, and writing NULL straight
 * into *items would lose the only pointer to it (a leak, and every
 * account gone). Returns 1 on success, 0 if out of memory. */
static int grow(void **items, size_t *capacity, size_t count, size_t item_size)
{
    if (count < *capacity)
        return 1;
    size_t new_capacity = *capacity == 0 ? 16 : *capacity * 2;
    void *bigger = realloc(*items, new_capacity * item_size);
    if (bigger == NULL)
        return 0;                               /* *items is untouched and still valid */
    *items = bigger;
    *capacity = new_capacity;
    return 1;
}

/* Adds entry to the bank's list of entries and updates the account(s) it
 * touches; an OPEN entry adds a new account. The entry must already have
 * passed bank_check(). Returns 1 on success, or 0 if memory ran out or
 * an account in the entry does not exist (which bank_check() rules out);
 * in either case nothing was changed. */
int bank_apply(Bank *bank, const Entry *entry)
{
    void *items;

    /* Grow both arrays first, so that a failure leaves nothing half done. */
    items = bank->entries;
    if (!grow(&items, &bank->entry_capacity, bank->entry_count, sizeof(Entry)))
        return 0;
    bank->entries = items;
    if (entry->type == ENTRY_OPEN) {
        items = bank->accounts;
        if (!grow(&items, &bank->account_capacity, bank->account_count, sizeof(Account)))
            return 0;
        bank->accounts = items;
    }

    /* A guard: never follow a NULL account pointer, even if a caller
     * forgot bank_check(). */
    Account *a = bank_find(bank, entry->account);
    Account *b = entry->type == ENTRY_TRANSFER ? bank_find(bank, entry->other) : NULL;
    if (entry->type != ENTRY_OPEN && a == NULL)
        return 0;
    if (entry->type == ENTRY_TRANSFER && b == NULL)
        return 0;
    switch (entry->type) {
    case ENTRY_OPEN:
        a = &bank->accounts[bank->account_count++];
        memset(a, 0, sizeof *a);
        a->number = entry->account;
        a->type = entry->account_type;
        strcpy(a->holder, entry->holder);
        strcpy(a->address, entry->address);
        a->opened = entry->date;
        a->balance = entry->amount;
        break;
    case ENTRY_DEPOSIT:
    case ENTRY_WITHDRAW:
        a->balance = entry->balance_after;
        break;
    case ENTRY_TRANSFER:
        a->balance = entry->balance_after;
        b->balance = entry->other_balance_after;
        break;
    case ENTRY_INTEREST:
        a->balance = entry->balance_after;
        a->last_interest_month = entry->month;  /* the cache of "already paid this month" */
        break;
    case ENTRY_EDIT:
        strcpy(a->holder, entry->holder);
        strcpy(a->address, entry->address);
        break;
    case ENTRY_CLOSE:
        a->is_closed = 1;
        break;
    }
    bank->entries[bank->entry_count++] = *entry;
    return 1;
}

/* Fills in the fields of a new entry that the bank decides, not the
 * teller: seq (the next entry number) and the balance(s) after it. The
 * menu then holds an entry exactly like the one the loader will later read
 * back from the file, and both go through the same bank_check().
 * Returns 1, or 0 if an account in the entry does not exist (the balance
 * fields are then 0 and bank_check reports RULE_NO_ACCOUNT). */
int bank_prepare(const Bank *bank, Entry *entry)
{
    entry->seq = (long)bank->entry_count + 1;
    entry->balance_after = bank_balance_after(bank, entry, 0);
    entry->other_balance_after = 0;
    if (entry->type == ENTRY_TRANSFER)
        entry->other_balance_after = bank_balance_after(bank, entry, 1);
    if (entry->type != ENTRY_OPEN && bank_find_const(bank, entry->account) == NULL)
        return 0;
    if (entry->type == ENTRY_TRANSFER && bank_find_const(bank, entry->other) == NULL)
        return 0;
    return 1;
}
