/* report.h - the printed tables: the account list, search results, an
 * account statement and the daily transaction report. They only read the
 * bank; nothing here changes it. */
#ifndef REPORT_H
#define REPORT_H

#include "bank.h"

void print_accounts(const Bank *bank);                        /* menu 6 */
void print_search(const Bank *bank, const char *text);        /* menu 7 */
void print_statement(const Bank *bank, const Account *account);   /* menu 5 */
void print_daily_report(const Bank *bank, Date day);          /* menu 8 */

#endif
