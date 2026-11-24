/* report.c - the account list, search, statements and the daily report.
 * Every amount is printed with money_format(), so the screen always shows
 * "1,234.50" while the file and the program work in whole cents. */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "report.h"

#define ROW_SIZE 160

/* Prints one table row built from fmt like printf(), then a newline, but
 * first removes spaces at the end of the row. A row with an empty Debit
 * or Credit column would otherwise end in invisible spaces, which make
 * the expected-output files of the tests hard to read and easy to break
 * in an editor that trims them. */
static void print_row(const char *fmt, ...)
{
    char row[ROW_SIZE];
    va_list args;

    va_start(args, fmt);
    vsnprintf(row, sizeof row, fmt, args);
    va_end(args);
    size_t len = strlen(row);
    while (len > 0 && row[len - 1] == ' ')
        row[--len] = '\0';
    puts(row);
}

/* Returns 1 if word appears anywhere in text, ignoring upper and lower case.
 * strcasestr() and stristr() would do this, but they are not standard C:
 * for each starting place in text, compare the letters of word one by one.
 * (From the personal-diary mini project.) */
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

/* Prints the column headings of the account list and the search results. */
static void print_account_header(void)
{
    print_row("%-7s %-30s %-7s %-6s %15s", "Account", "Holder", "Type", "Status", "Balance");
}

/* Prints one account as a row of the account list or the search results. */
static void print_account_row(const Account *a)
{
    char balance[MONEY_TEXT_SIZE];

    money_format(a->balance, balance, sizeof balance);
    print_row("%-7d %-30s %-7s %-6s %15s", a->number, a->holder,
              a->type == ACCOUNT_SAVINGS ? "Savings" : "Current",
              a->is_closed ? "closed" : "open", balance);
}

/* Prints every account in number order, then how many are open and
 * closed and the money held (the sum of all balances). */
void print_accounts(const Bank *bank)
{
    int open = 0;
    char held[MONEY_TEXT_SIZE];

    printf("\n");
    if (bank->account_count == 0) {
        printf("No accounts yet.\n");
        return;
    }
    print_account_header();
    for (size_t i = 0; i < bank->account_count; i++) {
        print_account_row(&bank->accounts[i]);
        if (!bank->accounts[i].is_closed)
            open++;
    }
    money_format(bank_money_held(bank), held, sizeof held);
    printf("%d account(s): %d open, %d closed. Money held: %s.\n",
           (int)bank->account_count, open, (int)bank->account_count - open, held);
}

/* Prints the accounts whose holder name contains text, ignoring case, in
 * the same table as the account list. */
void print_search(const Bank *bank, const char *text)
{
    int found = 0;

    printf("\n");
    for (size_t i = 0; i < bank->account_count; i++) {
        const Account *a = &bank->accounts[i];
        if (!contains_ignore_case(a->holder, text))
            continue;
        if (found++ == 0)
            print_account_header();
        print_account_row(a);
    }
    if (found == 0)
        printf("No accounts match \"%s\".\n", text);
    else
        printf("%d account(s) found.\n", found);
}

/* Describes entry e as seen from account number: "Cash deposit",
 * "Transfer to 100002" or "Transfer from 100003" (a transfer looks
 * different from each end), "Interest for 2026-12", and so on. Also sets
 * *debit (money out) and *credit (money in) to the amounts for the
 * Debit and Credit columns; both are 0 when no money moved. */
static void describe(const Entry *e, int number, char *text, size_t size, Cents *debit, Cents *credit)
{
    char month[DATE_TEXT_SIZE];

    *debit = 0;
    *credit = 0;
    switch (e->type) {
    case ENTRY_OPEN:
        snprintf(text, size, "Opening deposit");
        *credit = e->amount;
        break;
    case ENTRY_DEPOSIT:
        snprintf(text, size, "Cash deposit");
        *credit = e->amount;
        break;
    case ENTRY_WITHDRAW:
        snprintf(text, size, "Cash withdrawal");
        *debit = e->amount;
        break;
    case ENTRY_TRANSFER:
        if (number == e->account) {
            snprintf(text, size, "Transfer to %d", e->other);
            *debit = e->amount;
        } else {
            snprintf(text, size, "Transfer from %d", e->account);
            *credit = e->amount;
        }
        break;
    case ENTRY_INTEREST:
        date_format_month(e->month, month, sizeof month);
        snprintf(text, size, "Interest for %s", month);
        *credit = e->amount;
        break;
    case ENTRY_EDIT:
        snprintf(text, size, "Details updated");
        break;
    case ENTRY_CLOSE:
        snprintf(text, size, "Account closed");
        break;
    }
}

/* Writes amount into buf with money_format(), or an empty string when it
 * is 0, so an unused Debit or Credit column stays blank. */
static void format_column(Cents amount, char *buf, size_t size)
{
    if (amount == 0)
        buf[0] = '\0';
    else
        money_format(amount, buf, size);
}

/* Prints the account's details and every entry that touches it, in ledger
 * order, with a running balance, then the final balance. Works for closed
 * accounts too. The running balance is added up here from the amounts,
 * so the statement also shows that the stored balances add up. */
void print_statement(const Bank *bank, const Account *account)
{
    char date[DATE_TEXT_SIZE], text[48];
    char debit_text[MONEY_TEXT_SIZE], credit_text[MONEY_TEXT_SIZE], balance_text[MONEY_TEXT_SIZE];
    Cents balance = 0, debit, credit;

    date_format(account->opened, date, sizeof date);
    printf("\nAccount %d (%s, %s)\n", account->number,
           account->type == ACCOUNT_SAVINGS ? "Savings" : "Current",
           account->is_closed ? "closed" : "open");
    printf("Holder:  %s\nAddress: %s\nOpened:  %s\n\n", account->holder, account->address, date);
    print_row("%-10s  %4s  %-24s %12s %12s %14s", "Date", "No.", "Description", "Debit", "Credit", "Balance");
    for (size_t i = 0; i < bank->entry_count; i++) {
        const Entry *e = &bank->entries[i];
        if (e->account != account->number &&
            !(e->type == ENTRY_TRANSFER && e->other == account->number))
            continue;
        describe(e, account->number, text, sizeof text, &debit, &credit);
        balance += credit - debit;
        date_format(e->date, date, sizeof date);
        format_column(debit, debit_text, sizeof debit_text);
        format_column(credit, credit_text, sizeof credit_text);
        money_format(balance, balance_text, sizeof balance_text);
        print_row("%-10s  %4ld  %-24s %12s %12s %14s", date, e->seq, text,
                  debit_text, credit_text, balance_text);
    }
    money_format(account->balance, balance_text, sizeof balance_text);
    printf("Balance: %s\n", balance_text);
}

/* Prints one total line of the daily report, such as
 * "Deposits:         6, total 14,351.25". */
static void print_total(const char *label, int count, Cents total)
{
    char text[MONEY_TEXT_SIZE];

    money_format(total, text, sizeof text);
    printf("%-15s %3d, total %s\n", label, count, text);
}

/* Prints every entry dated day, one row per entry and account (so a
 * transfer gives two rows with the same number), then the number and
 * total of deposits (opening deposits included), withdrawals, transfers
 * and interest, and the net change in the money the bank holds. */
void print_daily_report(const Bank *bank, Date day)
{
    char date[DATE_TEXT_SIZE], text[48], debit_text[MONEY_TEXT_SIZE], credit_text[MONEY_TEXT_SIZE];
    int deposits = 0, withdrawals = 0, transfers = 0, interest = 0;
    Cents deposit_total = 0, withdrawal_total = 0, transfer_total = 0, interest_total = 0;
    Cents debit, credit;
    int printed = 0;

    date_format(day, date, sizeof date);
    printf("\n");
    for (size_t i = 0; i < bank->entry_count; i++) {
        const Entry *e = &bank->entries[i];
        if (date_compare(e->date, day) != 0)
            continue;
        if (printed++ == 0) {
            printf("Transactions on %s:\n", date);
            print_row("%4s  %-7s  %-24s %12s %12s", "No.", "Account", "Description", "Debit", "Credit");
        }
        /* A transfer is printed from both ends: first the source, then the target. */
        int ends = e->type == ENTRY_TRANSFER ? 2 : 1;
        for (int end = 0; end < ends; end++) {
            int number = end == 0 ? e->account : e->other;
            describe(e, number, text, sizeof text, &debit, &credit);
            format_column(debit, debit_text, sizeof debit_text);
            format_column(credit, credit_text, sizeof credit_text);
            print_row("%4ld  %-7d  %-24s %12s %12s", e->seq, number, text, debit_text, credit_text);
        }
        switch (e->type) {
        case ENTRY_OPEN:
        case ENTRY_DEPOSIT:  deposits++;    deposit_total += e->amount;    break;
        case ENTRY_WITHDRAW: withdrawals++; withdrawal_total += e->amount; break;
        case ENTRY_TRANSFER: transfers++;   transfer_total += e->amount;   break;
        case ENTRY_INTEREST: interest++;    interest_total += e->amount;   break;
        default:             break;
        }
    }
    if (printed == 0) {
        printf("No transactions on %s.\n", date);
        return;
    }
    printf("\n");
    print_total("Deposits:", deposits, deposit_total);
    print_total("Withdrawals:", withdrawals, withdrawal_total);
    print_total("Transfers:", transfers, transfer_total);
    print_total("Interest paid:", interest, interest_total);

    /* Transfers move money between two accounts of the same bank, so they
     * add to one balance exactly what they take from the other: they
     * cancel out and are left out of the net change. */
    money_format(deposit_total - withdrawal_total + interest_total, text, sizeof text);
    printf("Net change in money held: %s\n", text);
}
