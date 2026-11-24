/* unit_tests.c - tests for the bank's modules, linked against the same
 * bankcore library as the program, without main.c and its menu.
 * Prints "N checks, F failed" and returns 1 if anything failed. */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>

#include "bank.h"
#include "date.h"
#include "ledger.h"
#include "money.h"

/* Counts every check and every failure, and names a failing check with its
 * line, so the output says exactly what went wrong. A struct passed by
 * pointer would avoid the two file-level counters, but a test program
 * this size reads more easily with them (they are not in src/). */
static int checks = 0, failures = 0;
#define CHECK(cond) \
    do { checks++; if (!(cond)) { failures++; printf("FAILED line %d: %s\n", __LINE__, #cond); } } while (0)

/* Checks money_parse() on one input: whether it is accepted and, if so,
 * the value in cents. */
static void check_parse(const char *text, int ok, Cents cents)
{
    Cents value = -1;
    int got = money_parse(text, &value);

    CHECK(got == ok);
    if (ok)
        CHECK(value == cents);
    else
        CHECK(value == -1);                     /* untouched on failure */
    if (got != ok)
        printf("  money_parse(\"%s\")\n", text);
}

/* Checks money_format() on one value. */
static void check_format(Cents value, const char *want)
{
    char buf[MONEY_TEXT_SIZE];

    money_format(value, buf, sizeof buf);
    CHECK(strcmp(buf, want) == 0);
    if (strcmp(buf, want) != 0)
        printf("  money_format(%lld) gave \"%s\", expected \"%s\"\n", value, buf, want);
}

static void test_money(void)
{
    check_parse("250", 1, 25000);
    check_parse("250.7", 1, 25070);
    check_parse(" 250.75 ", 1, 25075);
    check_parse("0.01", 1, 1);
    check_parse("1000000", 1, 100000000);
    check_parse("1000000.01", 0, 0);
    check_parse("0", 0, 0);
    check_parse("0.00", 0, 0);
    check_parse("12.505", 0, 0);
    check_parse("250.", 0, 0);
    check_parse(".5", 0, 0);
    check_parse("-5", 0, 0);
    check_parse("+5", 0, 0);
    check_parse("1,000", 0, 0);
    check_parse("1e3", 0, 0);
    check_parse("nan", 0, 0);
    check_parse("", 0, 0);
    check_parse("12abc", 0, 0);
    check_parse("99999999999999999999", 0, 0);

    check_format(0, "0.00");
    check_format(5, "0.05");
    check_format(99, "0.99");
    check_format(100, "1.00");
    check_format(123456789, "1,234,567.89");
    check_format(-39950, "-399.50");
    check_format(-50000, "-500.00");
    check_format(99999999999LL, "999,999,999.99");

    CHECK(interest_for_month(200, 300) == 1);       /* exactly 0.5 cent rounds up */
    CHECK(interest_for_month(199, 300) == 0);
    CHECK(interest_for_month(185075, 300) == 463);
    CHECK(interest_for_month(460050, 300) == 1150);
    CHECK(interest_for_month(501250, 300) == 1253);
    CHECK(interest_for_month(1, 300) == 0);
    CHECK(interest_for_month(0, 300) == 0);
    CHECK(interest_for_month(-500, 300) == 0);
}

static void test_dates(void)
{
    Date d = { 0, 0, 0 };

    CHECK(date_parse("2028-02-29", &d) && d.year == 2028 && d.month == 2 && d.day == 29);
    CHECK(!date_parse("2027-02-29", &d));
    CHECK(!date_parse("2026-13-01", &d));
    CHECK(!date_parse("2026-1-01", &d));
    CHECK(!date_parse("2026-01-01x", &d));
    CHECK(!date_parse("1899-12-31", &d));
    CHECK(date_parse("2026-12-31", &d) && date_month_key(d) == 202612);
}

/* Builds an entry for the test bank, dated 2026-12-01 unless changed. */
static Entry make(EntryType type, int account, Cents amount)
{
    Entry e;

    memset(&e, 0, sizeof e);
    e.type = type;
    e.date.year = 2026;
    e.date.month = 12;
    e.date.day = 1;
    e.account = account;
    e.amount = amount;
    return e;
}

/* Prepares, checks and applies entry, as the menu does (without the file).
 * Returns the Rule; the entry is applied only if it is RULE_OK. */
static Rule add(Bank *bank, Entry e)
{
    bank_prepare(bank, &e);
    Rule rule = bank_check(bank, &e);
    if (rule == RULE_OK)
        CHECK(bank_apply(bank, &e));
    return rule;
}

/* Prepares entry and returns bank_check()'s verdict without applying it. */
static Rule try_entry(const Bank *bank, Entry e)
{
    bank_prepare(bank, &e);
    return bank_check(bank, &e);
}

/* Opens an account in the test bank and checks that it worked. */
static void open_account(Bank *bank, AccountType type, const char *name, Cents amount)
{
    Entry e = make(ENTRY_OPEN, bank_next_number(bank), amount);
    e.account_type = type;
    strcpy(e.holder, name);
    strcpy(e.address, "1 Test Street");
    CHECK(add(bank, e) == RULE_OK);
}

/* One case per Rule value, on a small bank built in memory:
 * 100001 savings 2,000.00, 100002 current 100.00, 100003 savings 0.00 then closed. */
static void test_rules(void)
{
    Bank bank;
    Entry e;

    bank_init(&bank);
    open_account(&bank, ACCOUNT_SAVINGS, "Sam Saver", 200000);
    open_account(&bank, ACCOUNT_CURRENT, "Cora Current", 100000);
    open_account(&bank, ACCOUNT_SAVINGS, "Ned Closed", 100000);
    CHECK(bank_next_number(&bank) == 100004);
    CHECK(add(&bank, make(ENTRY_WITHDRAW, 100003, 100000)) == RULE_OK);
    CHECK(add(&bank, make(ENTRY_CLOSE, 100003, 0)) == RULE_OK);
    CHECK(add(&bank, make(ENTRY_WITHDRAW, 100002, 90000)) == RULE_OK);   /* 100002 now 100.00 */

    CHECK(try_entry(&bank, make(ENTRY_DEPOSIT, 100001, 100)) == RULE_OK);
    CHECK(try_entry(&bank, make(ENTRY_DEPOSIT, 100009, 100)) == RULE_NO_ACCOUNT);
    CHECK(try_entry(&bank, make(ENTRY_DEPOSIT, 100003, 100)) == RULE_CLOSED);

    e = make(ENTRY_OPEN, 100005, 100000);
    strcpy(e.holder, "Al");
    strcpy(e.address, "x");
    CHECK(try_entry(&bank, e) == RULE_WRONG_NUMBER);
    e.account = 100004;
    CHECK(try_entry(&bank, e) == RULE_OK);
    e.amount = 99999;
    CHECK(try_entry(&bank, e) == RULE_OPENING_TOO_SMALL);
    e.amount = 100000;
    strcpy(e.holder, "A");                      /* one character is too short */
    CHECK(try_entry(&bank, e) == RULE_BAD_TEXT);
    strcpy(e.holder, "A|B");
    CHECK(try_entry(&bank, e) == RULE_BAD_TEXT);

    CHECK(try_entry(&bank, make(ENTRY_DEPOSIT, 100001, 0)) == RULE_BAD_AMOUNT);
    CHECK(try_entry(&bank, make(ENTRY_DEPOSIT, 100001, MAX_AMOUNT + 1)) == RULE_BAD_AMOUNT);

    /* Limits: savings to exactly 0.00 and current to exactly -500.00 are allowed. */
    CHECK(try_entry(&bank, make(ENTRY_WITHDRAW, 100001, 200000)) == RULE_OK);
    CHECK(try_entry(&bank, make(ENTRY_WITHDRAW, 100001, 200001)) == RULE_INSUFFICIENT);
    CHECK(try_entry(&bank, make(ENTRY_WITHDRAW, 100002, 60000)) == RULE_OK);
    CHECK(try_entry(&bank, make(ENTRY_WITHDRAW, 100002, 60001)) == RULE_INSUFFICIENT);

    /* Daily limit: 100001 gets 10,000.00 more, then 5,000.00 in a day is
     * allowed, 0.01 more is not, and the next day starts again. */
    CHECK(add(&bank, make(ENTRY_DEPOSIT, 100001, 1000000)) == RULE_OK);
    CHECK(add(&bank, make(ENTRY_WITHDRAW, 100001, 300000)) == RULE_OK);
    CHECK(try_entry(&bank, make(ENTRY_WITHDRAW, 100001, 200000)) == RULE_OK);
    CHECK(try_entry(&bank, make(ENTRY_WITHDRAW, 100001, 200001)) == RULE_DAILY_LIMIT);
    CHECK(bank_withdrawn_on(&bank, 100001, make(ENTRY_DEPOSIT, 0, 0).date) == 300000);
    e = make(ENTRY_WITHDRAW, 100001, 200001);
    e.date.day = 2;
    CHECK(try_entry(&bank, e) == RULE_OK);

    e = make(ENTRY_TRANSFER, 100001, 100);
    e.other = 100001;
    CHECK(try_entry(&bank, e) == RULE_SAME_ACCOUNT);
    e.other = 100003;
    CHECK(try_entry(&bank, e) == RULE_CLOSED);
    e.other = 100002;
    CHECK(try_entry(&bank, e) == RULE_OK);
    e.amount = 900001;                          /* 100001 holds 9,000.00 */
    CHECK(try_entry(&bank, e) == RULE_INSUFFICIENT);

    e = make(ENTRY_INTEREST, 100002, 0);
    e.month = 202612;
    CHECK(try_entry(&bank, e) == RULE_NOT_SAVINGS);
    e.account = 100001;
    e.amount = interest_for_month(900000, SAVINGS_RATE_BP);   /* 22.50 */
    CHECK(e.amount == 2250);
    CHECK(try_entry(&bank, e) == RULE_OK);
    e.amount = 2251;
    CHECK(try_entry(&bank, e) == RULE_WRONG_INTEREST);
    e.amount = 2250;
    CHECK(add(&bank, e) == RULE_OK);
    CHECK(try_entry(&bank, e) == RULE_INTEREST_PAID);

    CHECK(try_entry(&bank, make(ENTRY_CLOSE, 100001, 0)) == RULE_NOT_EMPTY);

    e = make(ENTRY_DEPOSIT, 100001, 100);
    e.date.month = 11;
    CHECK(try_entry(&bank, e) == RULE_DATE_BACKWARDS);

    e = make(ENTRY_DEPOSIT, 100001, 100);
    bank_prepare(&bank, &e);
    e.balance_after++;
    CHECK(bank_check(&bank, &e) == RULE_BALANCE_MISMATCH);
    bank_free(&bank);

    /* The largest balance: 999,999,999.99 is allowed, one cent more is not. */
    bank_init(&bank);
    open_account(&bank, ACCOUNT_CURRENT, "Rich Person", MAX_AMOUNT);
    for (int i = 0; i < 998; i++)
        CHECK(add(&bank, make(ENTRY_DEPOSIT, 100001, MAX_AMOUNT)) == RULE_OK);
    CHECK(add(&bank, make(ENTRY_DEPOSIT, 100001, MAX_AMOUNT - 1)) == RULE_OK);
    CHECK(bank.accounts[0].balance == MAX_BALANCE);
    CHECK(try_entry(&bank, make(ENTRY_DEPOSIT, 100001, 1)) == RULE_ABOVE_MAX);
    char text[MONEY_TEXT_SIZE];
    money_format(bank.accounts[0].balance, text, sizeof text);
    CHECK(strcmp(text, "999,999,999.99") == 0);
    CHECK(bank.entry_count == 1000);            /* the arrays grew by doubling */
    bank_free(&bank);
}

/* Returns 1 if two entries have the same fields. */
static int same_entry(const Entry *a, const Entry *b)
{
    return a->seq == b->seq && date_compare(a->date, b->date) == 0 && a->type == b->type &&
           a->account == b->account && a->other == b->other && a->account_type == b->account_type &&
           a->amount == b->amount && a->balance_after == b->balance_after &&
           a->other_balance_after == b->other_balance_after && a->month == b->month &&
           strcmp(a->holder, b->holder) == 0 && strcmp(a->address, b->address) == 0;
}

/* entry_to_line() then line_to_entry() must give back the same fields,
 * for one entry of each type. */
static void test_round_trip(void)
{
    Entry in[7], out;
    char line[LEDGER_LINE], why[REASON_SIZE];

    for (int i = 0; i < 7; i++)
        in[i] = make((EntryType)i, 100001, 0);
    in[ENTRY_OPEN].account_type = ACCOUNT_CURRENT;
    in[ENTRY_OPEN].amount = in[ENTRY_OPEN].balance_after = 250050;
    strcpy(in[ENTRY_OPEN].holder, "Ben Kim");
    strcpy(in[ENTRY_OPEN].address, "4 Mill Lane, Springfield");
    in[ENTRY_DEPOSIT].amount = 25075;
    in[ENTRY_DEPOSIT].balance_after = 175075;
    in[ENTRY_WITHDRAW].amount = 290000;
    in[ENTRY_WITHDRAW].balance_after = -39950;
    in[ENTRY_TRANSFER].other = 100002;
    in[ENTRY_TRANSFER].amount = 39950;
    in[ENTRY_TRANSFER].balance_after = 460050;
    in[ENTRY_TRANSFER].other_balance_after = 0;
    in[ENTRY_INTEREST].month = 202612;
    in[ENTRY_INTEREST].amount = 463;
    in[ENTRY_INTEREST].balance_after = 185538;
    strcpy(in[ENTRY_EDIT].holder, "Ana Lopez");
    strcpy(in[ENTRY_EDIT].address, "14 Harbour Road, Springfield");
    for (int i = 0; i < 7; i++) {
        in[i].seq = i + 1;
        CHECK(entry_to_line(&in[i], line, sizeof line));
        CHECK(line_to_entry(line, &out, why, sizeof why));
        CHECK(same_entry(&in[i], &out));
    }

    /* A few lines that must be refused, with the loader's reasons. */
    strcpy(line, "4|2026-12-01|DEPO|100001|25075|175075");
    CHECK(!line_to_entry(line, &out, why, sizeof why) && strcmp(why, "unknown entry type \"DEPO\"") == 0);
    strcpy(line, "8|2026-12-01|WDL|100003|400000");
    CHECK(!line_to_entry(line, &out, why, sizeof why) &&
          strcmp(why, "wrong number of fields for a WDL entry") == 0);
    strcpy(line, "4|2026-12-01|DEP|100001|-5|175075");
    CHECK(!line_to_entry(line, &out, why, sizeof why) && strcmp(why, "not a valid number in field 5") == 0);
    strcpy(line, "4|2026-02-30|DEP|100001|5|175075");
    CHECK(!line_to_entry(line, &out, why, sizeof why) && strcmp(why, "not a valid date") == 0);
    strcpy(line, "13|2026-12-01|INT|100001|2026-11|463|185538");
    CHECK(!line_to_entry(line, &out, why, sizeof why) &&
          strcmp(why, "the interest month does not match the entry date") == 0);
}

int main(void)
{
    test_money();
    test_dates();
    test_rules();
    test_round_trip();
    printf("%d checks, %d failed\n", checks, failures);
    return failures > 0;
}
