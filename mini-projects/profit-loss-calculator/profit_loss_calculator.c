/*
 * profit_loss_calculator.c - profit or loss on a sale, the selling price
 * for a target profit, and the break-even quantity.
 *
 * Money is kept as whole cents in a long long, so totals are exact.
 * Only percentages use double.
 *
 * Build with GCC or Clang:
 *     gcc -std=c11 -Wall -Wextra -o profit_loss_calculator profit_loss_calculator.c
 * Build with Visual Studio (Developer Command Prompt):
 *     cl /W4 profit_loss_calculator.c
 * Run:
 *     ./profit_loss_calculator      (Linux, macOS, WSL)
 *     profit_loss_calculator.exe    (Windows)
 */
#include <ctype.h>
#include <errno.h>
#include <math.h>      /* isfinite() only: a macro, so no -lm is needed */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 64
#define MAX_MONEY_CENTS 100000000000LL   /* 1,000,000,000.00: keeps every total inside long long */
#define MAX_QUANTITY    1000000          /* 10^11 cents x 10^6 units = at most 10^17 cents */

static int       read_line(char *buf, size_t size);
static int       read_int(const char *prompt, int lo, int hi, int *value);
static int       read_double(const char *prompt, double *value);
static int       parse_money(const char *text, long long *cents);
static int       read_money(const char *prompt, long long *cents);
static void      print_money(long long cents);
static long long round_to_cents(double amount_in_cents);
static void      sale_report(long long cost, long long price, int quantity);
static long long price_for_percent(long long cost, double percent);
static long long break_even_units(long long fixed, long long unit_cost, long long price);

int main(void)
{
    int choice = 0, quantity = 0, ok = 1;
    long long cost = 0, price = 0, fixed = 0, units = 0;
    double percent = 0.0;

    printf("Profit and Loss Calculator\n==========================\n");

    while (ok) {
        printf("\n1. Profit or loss on a sale\n"
               "2. Selling price for a target profit or loss\n"
               "3. Break-even quantity\n"
               "0. Quit\n");
        if (!read_int("Choose an option (0-3): ", 0, 3, &choice))
            break;                              /* input ended */
        switch (choice) {
        case 0:
            printf("\nGoodbye!\n");
            return 0;
        case 1:
            ok = read_money("Cost price per unit: ", &cost) &&
                 read_money("Selling price per unit: ", &price) &&
                 read_int("Quantity sold: ", 1, MAX_QUANTITY, &quantity);
            if (ok)
                sale_report(cost, price, quantity);
            break;
        case 2:
            ok = read_money("Cost price: ", &cost);
            while (ok) {                        /* ask until the percentage is in range */
                ok = read_double("Target percentage (negative for a loss): ", &percent);
                if (!ok || (percent >= -100.0 && percent <= 1000.0))
                    break;
                printf("Please enter a percentage from -100 to 1000.\n");
            }
            if (!ok)
                break;
            if (percent > 0)
                printf("Selling price for %.6g%% profit: ", percent);
            else if (percent < 0)
                printf("Selling price for %.6g%% loss: ", -percent);
            else
                printf("Selling price for no profit and no loss: ");
            print_money(price_for_percent(cost, percent));
            printf("\n");
            break;
        default:                                /* 3: break-even */
            ok = read_money("Fixed costs (rent, equipment, ...): ", &fixed) &&
                 read_money("Cost per unit: ", &cost) &&
                 read_money("Selling price per unit: ", &price);
            if (!ok)
                break;
            units = break_even_units(fixed, cost, price);
            if (units < 0) {
                printf("Each unit earns nothing toward fixed costs (selling price <= cost per unit),\n"
                       "so sales can never cover them.\n");
            } else {
                printf("Each unit earns ");
                print_money(price - cost);
                printf(" toward fixed costs.\nBreak-even quantity: %lld units\n", units);
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

/* Prints prompt and reads any finite real number into *value.
 * Rejects an empty line, trailing junk (12abc), nan, inf and values too
 * large for a double. Returns 1 on success, 0 at end of input. */
static int read_double(const char *prompt, double *value)
{
    char buf[LINE_SIZE];
    char *end;

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;
        errno = 0;
        double x = strtod(buf, &end);
        while (*end == ' ' || *end == '\t')
            end++;                              /* allow trailing spaces */
        if (end != buf && *end == '\0' && errno != ERANGE && isfinite(x)) {
            *value = x;
            return 1;
        }
        printf("Please enter a number, for example 12.5.\n");
    }
}


/* Converts text such as "12", "12.5" or "12.50" into cents (1200, 1250, 1250).
 * Accepts: spaces or tabs around the number, at least one digit, then
 * optionally a '.' followed by exactly 1 or 2 digits.
 * Rejects: a sign, a comma, a third decimal place, "12.", ".5", any other
 * character, and anything above MAX_MONEY_CENTS.
 * Returns 1 on success, 0 otherwise. */
static int parse_money(const char *text, long long *cents)
{
    const char *p = text;
    long long value = 0;
    int digits = 0;

    while (*p == ' ' || *p == '\t')
        p++;
    while (isdigit((unsigned char)*p)) {
        if (value > MAX_MONEY_CENTS / 100)
            return 0;                           /* already too big: stop before it can overflow */
        value = value * 10 + (*p - '0');
        digits++;
        p++;
    }
    if (digits == 0)
        return 0;
    value *= 100;                               /* whole units to cents */
    if (*p == '.') {
        p++;
        if (!isdigit((unsigned char)*p))
            return 0;                           /* "12." has no digit after the dot */
        value += (*p - '0') * 10;
        p++;
        if (isdigit((unsigned char)*p)) {
            value += *p - '0';
            p++;
        }
    }
    while (*p == ' ' || *p == '\t')
        p++;
    if (*p != '\0' || value > MAX_MONEY_CENTS)
        return 0;                               /* a third decimal, a comma, a letter... */
    *cents = value;
    return 1;
}

/* Prints prompt and reads an amount of money into *cents, asking again
 * until parse_money() accepts it. Returns 1 on success, 0 at end of input. */
static int read_money(const char *prompt, long long *cents)
{
    char buf[LINE_SIZE];

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;
        if (parse_money(buf, cents))
            return 1;
        printf("Please enter an amount such as 12.50 (0 to 1000000000, at most 2 decimal places).\n");
    }
}

/* Prints cents as <whole>.<two digits>: 125000 as 1250.00, 5 as 0.05.
 * A negative amount gets a leading '-'. Prints no newline. */
static void print_money(long long cents)
{
    if (cents < 0) {
        printf("-");
        cents = -cents;
    }
    printf("%lld.%02lld", cents / 100, cents % 100);
}

/* Rounds a number of cents to the nearest whole cent, halves away from zero.
 * math.h llround() does the same, but with GCC on Linux it needs -lm;
 * this one line is enough here. */
static long long round_to_cents(double amount_in_cents)
{
    double x = amount_in_cents;
    return x >= 0 ? (long long)(x + 0.5) : -(long long)(-x + 0.5);
}

/* Prints total cost, total revenue, the result (profit, loss or neither)
 * and the percentages: markup (on cost) and margin (on selling price) for
 * a profit, loss on cost for a loss. A percentage of a zero total is "not defined". */
static void sale_report(long long cost, long long price, int quantity)
{
    long long total_cost = cost * quantity;
    long long revenue = price * quantity;
    long long difference = revenue - total_cost;

    printf("%-16s", "Total cost:");
    print_money(total_cost);
    printf("\n%-16s", "Total revenue:");
    print_money(revenue);
    printf("\n%-16s", "Result:");
    if (difference > 0) {
        printf("Profit of ");
        print_money(difference);
        printf("\n%-36s", "Markup (profit on cost):");
        if (total_cost == 0)
            printf("not defined (cost price is 0)\n");
        else
            printf("%.2f%%\n", (double)difference * 100.0 / total_cost);
        printf("%-36s", "Margin (profit on selling price):");
        if (revenue == 0)
            printf("not defined (selling price is 0)\n");
        else
            printf("%.2f%%\n", (double)difference * 100.0 / revenue);
    } else if (difference < 0) {
        printf("Loss of ");
        print_money(-difference);
        printf("\n%-16s%.2f%%\n", "Loss on cost:", (double)-difference * 100.0 / total_cost);
    } else {
        printf("No profit, no loss\n");
    }
}

/* Returns the selling price, in cents, that gives percent profit on cost
 * (a negative percent is a loss), rounded to the nearest cent. */
static long long price_for_percent(long long cost, double percent)
{
    return round_to_cents((double)cost * (100.0 + percent) / 100.0);
}

/* Returns how many whole units must be sold to cover the fixed costs,
 * 0 if there are none, or -1 if each unit earns nothing (price <= unit_cost).
 * It rounds up with integer ceiling division (a + b - 1) / b: for example
 * 5000.00 / 15.00 = 333.33; 333 units fall short, so the answer is 334. */
static long long break_even_units(long long fixed, long long unit_cost, long long price)
{
    long long per_unit = price - unit_cost;

    if (per_unit <= 0)
        return -1;
    if (fixed == 0)
        return 0;
    return (fixed + per_unit - 1) / per_unit;
}
