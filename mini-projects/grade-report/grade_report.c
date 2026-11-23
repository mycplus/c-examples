/*
 * grade_report.c - a teacher's report card for one class.
 *
 * Enter the subjects, then each student's name and marks (0 to 100).
 * The program prints every student's marks, total, average and letter
 * grade, each subject's average, highest and lowest mark, the class
 * average, the top student and how many students got each grade.
 *
 * Build with GCC or Clang:
 *     gcc -std=c11 -Wall -Wextra -o grade_report grade_report.c
 * Build with Visual Studio (Developer Command Prompt):
 *     cl /W4 grade_report.c
 * Run:
 *     ./grade_report                       type the class in
 *     ./grade_report < tests/session.txt   see a full report without typing
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE    64
#define MAX_STUDENTS 30
#define MAX_SUBJECTS 6
#define NAME_SIZE    21   /* 20 characters + '\0' */
#define SUBJECT_SIZE 9    /* 8 characters + '\0' (fits the 8-wide column) */

static int    read_line(char *buf, size_t size);
static int    read_int(const char *prompt, int lo, int hi, int *value);
static int    read_text(const char *prompt, char *out, size_t out_size);
static char   letter_grade(double average);
static double average_of(int total, int count);
static void   print_report(char names[][NAME_SIZE], int marks[][MAX_SUBJECTS],
                           int students, char subjects[][SUBJECT_SIZE], int subject_count);
static void   print_subject_stats(int marks[][MAX_SUBJECTS], int students, int subject_count);
static void   print_class_summary(char names[][NAME_SIZE], int marks[][MAX_SUBJECTS],
                                  int students, int subject_count);

int main(void)
{
    /* Each name is a row of a 2D char array: the characters live inside
     * the array itself. Hangman's word list was an array of pointers to
     * string literals instead, which cannot be typed into. */
    char subjects[MAX_SUBJECTS][SUBJECT_SIZE];
    char names[MAX_STUDENTS][NAME_SIZE];
    int marks[MAX_STUDENTS][MAX_SUBJECTS];     /* one row per student, one column per subject */
    int subject_count = 0, students = 0;
    char prompt[LINE_SIZE];

    printf("Student Grade Report\n====================\n");

    /* ok turns 0 as soon as input ends; every loop below then stops. */
    int ok = read_int("How many subjects (1-6)? ", 1, MAX_SUBJECTS, &subject_count);
    for (int j = 0; ok && j < subject_count; j++) {
        snprintf(prompt, sizeof prompt, "Name of subject %d: ", j + 1);
        ok = read_text(prompt, subjects[j], SUBJECT_SIZE);
    }
    ok = ok && read_int("How many students (1-30)? ", 1, MAX_STUDENTS, &students);

    for (int i = 0; ok && i < students; i++) {
        printf("\n");
        snprintf(prompt, sizeof prompt, "Student %d name: ", i + 1);
        ok = read_text(prompt, names[i], NAME_SIZE);
        for (int j = 0; ok && j < subject_count; j++) {
            snprintf(prompt, sizeof prompt, "  %s mark (0-100): ", subjects[j]);
            ok = read_int(prompt, 0, 100, &marks[i][j]);
        }
    }
    if (!ok) {
        printf("\n\nGoodbye!\n");
        return 0;
    }

    print_report(names, marks, students, subjects, subject_count);
    print_subject_stats(marks, students, subject_count);
    print_class_summary(names, marks, students, subject_count);
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

/* Returns the letter grade for an average: A from 90, B from 80, C from 70,
 * D from 60, F below 60. The cutoffs are a lookup table, so adding a grade
 * means adding one entry. >= makes exactly 90.00 an A and 60.00 a D. */
static char letter_grade(double average)
{
    static const int  cutoffs[] = { 90, 80, 70, 60 };
    static const char letters[] = "ABCD";

    for (int k = 0; k < (int)(sizeof cutoffs / sizeof cutoffs[0]); k++)
        if (average >= cutoffs[k])
            return letters[k];
    return 'F';
}

/* Returns total / count as a double. Every average goes through here. */
static double average_of(int total, int count)
{
    return (double)total / count;   /* int / int would drop the fraction: 275 / 3 is 91 */
}

/* Prints the report card: a header, then one row per student with each
 * mark, the total, the average and the letter grade. A 2D array parameter
 * needs its column size, and students says how many rows are filled,
 * because inside the function the array is only a pointer. */
static void print_report(char names[][NAME_SIZE], int marks[][MAX_SUBJECTS],
                         int students, char subjects[][SUBJECT_SIZE], int subject_count)
{
    printf("\nREPORT CARD\n===========\n");
    printf("%-20s", "Name");
    for (int j = 0; j < subject_count; j++)
        printf("%8s", subjects[j]);
    printf("%8s%9s  %s\n", "Total", "Average", "Grade");

    for (int i = 0; i < students; i++) {        /* one row per student */
        int total = 0;
        printf("%-20s", names[i]);
        for (int j = 0; j < subject_count; j++) {
            printf("%8d", marks[i][j]);
            total += marks[i][j];
        }
        double average = average_of(total, subject_count);
        printf("%8d%9.2f  %c\n", total, average, letter_grade(average));
    }

    int width = 20 + 8 * subject_count + 8 + 9 + 7;   /* as long as the header */
    for (int k = 0; k < width; k++)
        printf("-");
    printf("\n");
}

/* Prints the Average, Highest and Lowest rows, one value per subject column.
 * For each column it loops down the rows. Highest and lowest start from the
 * first student's mark, not from 0 or 100: a lowest that started at 0 would
 * stay 0 for every subject. */
static void print_subject_stats(int marks[][MAX_SUBJECTS], int students, int subject_count)
{
    int highest[MAX_SUBJECTS], lowest[MAX_SUBJECTS];

    printf("%-20s", "Average");
    for (int j = 0; j < subject_count; j++) {   /* one column per subject */
        int sum = 0;
        highest[j] = lowest[j] = marks[0][j];
        for (int i = 0; i < students; i++) {
            sum += marks[i][j];
            if (marks[i][j] > highest[j])
                highest[j] = marks[i][j];
            if (marks[i][j] < lowest[j])
                lowest[j] = marks[i][j];
        }
        printf("%8.2f", average_of(sum, students));
    }
    printf("\n%-20s", "Highest");
    for (int j = 0; j < subject_count; j++)
        printf("%8d", highest[j]);
    printf("\n%-20s", "Lowest");
    for (int j = 0; j < subject_count; j++)
        printf("%8d", lowest[j]);
    printf("\n");
}

/* Prints the class average (all marks over students x subjects), the top
 * student by total and how many students got each grade. On a tie for the
 * top total, the student entered first wins, because only a bigger total
 * replaces the current best. */
static void print_class_summary(char names[][NAME_SIZE], int marks[][MAX_SUBJECTS],
                                int students, int subject_count)
{
    static const char grades[] = "ABCDF";
    int totals[MAX_STUDENTS];                   /* one total per student */
    int counts[5] = { 0 };
    int all = 0, top = 0;

    for (int i = 0; i < students; i++) {
        totals[i] = 0;
        for (int j = 0; j < subject_count; j++)
            totals[i] += marks[i][j];
        all += totals[i];
        if (totals[i] > totals[top])
            top = i;
        char grade = letter_grade(average_of(totals[i], subject_count));
        counts[strchr(grades, grade) - grades]++;   /* 'A' -> 0 ... 'F' -> 4 */
    }

    printf("\nClass average: %.2f\n", average_of(all, students * subject_count));
    printf("Top student:   %s (total %d, average %.2f)\n",
           names[top], totals[top], average_of(totals[top], subject_count));
    printf("Grade counts: ");
    for (int k = 0; k < 5; k++)
        printf(" %c: %d%s", grades[k], counts[k], k < 4 ? " " : "\n");
}
