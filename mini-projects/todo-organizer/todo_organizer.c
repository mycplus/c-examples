/*
 * todo_organizer.c - a to-do list kept in a text file, using dynamic memory.
 *
 * Each task has a text, a priority (high, medium or low), an optional due
 * date and a done mark. The list shows tasks still to do first, then by
 * priority, then by due date, and marks overdue tasks. Every change is
 * saved straight away. Each line of the file is one task:
 *     done|priority|due date or -|text       for example  0|1|2026-12-03|Write brief 13
 * The list has no fixed size: it grows with realloc() as tasks are added,
 * and each task's text takes exactly the memory it needs.
 *
 * Reused from earlier projects, copied unchanged:
 *   read_line, read_int                     unit_converter.c  (project 1)
 *   read_text                               grade_report.c    (project 5)
 *   is_leap_year, is_valid_date,
 *   today_string                            personal_diary.c  (project 10)
 *
 * Build with GCC or Clang:
 *     gcc -std=c11 -Wall -Wextra -o todo_organizer todo_organizer.c
 * Check for memory leaks (Linux):
 *     gcc -std=c11 -g -fsanitize=address -o todo_organizer todo_organizer.c
 * Build with Visual Studio (Developer Command Prompt):
 *     cl /W4 todo_organizer.c
 * Run:
 *     ./todo_organizer                      uses todo.txt and today's date
 *     ./todo_organizer --file work.txt      a second list
 */
#define _CRT_SECURE_NO_WARNINGS   /* fopen, strerror, localtime, remove, rename on MSVC */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LINE_SIZE      256        /* read_line buffer: room for a 200-character task */
#define TEXT_SIZE      201        /* typed task text: 200 characters + '\0' */
#define DATE_SIZE      11         /* "YYYY-MM-DD" + '\0' */
#define PATH_SIZE      260
#define FIRST_CAPACITY 4          /* the list's first allocation */
#define FIRST_LINE     16         /* read_file_line's first buffer: small on purpose */
#define DEFAULT_FILE   "todo.txt"

/* One task. */
typedef struct {
    char *text;               /* malloc'ed; owned by the list */
    int   priority;           /* 1 high, 2 medium, 3 low */
    char  due[DATE_SIZE];     /* "YYYY-MM-DD", or "" for no due date */
    bool  done;
} Task;

/* A growing array of tasks. The list owns the items array and every
 * task's text. The rule: every malloc has exactly one free. */
typedef struct {
    Task *items;              /* malloc'ed array, NULL until the first task */
    int   count;              /* tasks in use */
    int   capacity;           /* tasks the array has room for */
} TaskList;

static const char *const priority_names[4] = { "", "high", "medium", "low" };

static int   read_line(char *buf, size_t size);
static int   read_int(const char *prompt, int lo, int hi, int *value);
static int   read_text(const char *prompt, char *out, size_t out_size);
static int   is_leap_year(int year);
static int   is_valid_date(const char *text);
static void  today_string(char out[DATE_SIZE]);
static char *copy_string(const char *s);
static int   list_append(TaskList *list, Task task);
static void  list_remove(TaskList *list, int index);
static int   list_remove_done(TaskList *list);
static void  list_free(TaskList *list);
static int   compare_tasks(const void *a, const void *b);
static void  show_list(const TaskList *list, const char *today);
static int   read_file_line(FILE *f, char **line);
static int   parse_task_line(const char *line, Task *out);
static int   load_list(TaskList *list, const char *path);
static int   save_list(const TaskList *list, const char *path);
static int   read_due_date(char due[DATE_SIZE]);
static int   add_task(TaskList *list, const char *path);
static int   mark_done(TaskList *list, const char *path);
static int   delete_task(TaskList *list, const char *path);

int main(int argc, char *argv[])
{
    TaskList list = { NULL, 0, 0 };
    const char *path = DEFAULT_FILE;
    char today[DATE_SIZE] = "";
    int choice = 0, ok = 1;

    for (int i = 1; i < argc; i += 2) {         /* options come in pairs, any order */
        if (i + 1 < argc && strcmp(argv[i], "--file") == 0 && strlen(argv[i + 1]) < PATH_SIZE - 4) {
            path = argv[i + 1];
        } else if (i + 1 < argc && strcmp(argv[i], "--today") == 0 && is_valid_date(argv[i + 1])) {
            strcpy(today, argv[i + 1]);
        } else {
            fprintf(stderr, "Usage: todo_organizer [--file NAME] [--today YYYY-MM-DD]\n");
            return 1;
        }
    }
    if (today[0] == '\0')
        today_string(today);

    printf("To-Do Organizer\n===============\nToday is %s.\n", today);
    if (!load_list(&list, path))
        return 1;                               /* load_list has freed everything */

    while (ok) {
        printf("\n1. List tasks\n2. Add a task\n3. Mark a task as done\n4. Delete a task\n"
               "5. Remove completed tasks\n0. Quit\n");
        if (!read_int("Choose an option (0-5): ", 0, 5, &choice))
            break;
        switch (choice) {
        case 1: printf("\n"); show_list(&list, today); break;
        case 2: ok = add_task(&list, path); break;
        case 3: ok = mark_done(&list, path); break;
        case 4: ok = delete_task(&list, path); break;
        case 5: {
            int removed = list_remove_done(&list);
            if (removed == 0) {
                printf("There are no completed tasks.\n");
            } else {
                printf("Removed %d completed task%s.\n", removed, removed == 1 ? "" : "s");
                save_list(&list, path);
            }
            break;
        }
        default:
            printf("\nGoodbye!\n");
            list_free(&list);
            return 0;
        }
    }
    printf("\n\nGoodbye!\n");                  /* input ended */
    list_free(&list);
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

/* Returns a malloc'ed copy of s, or NULL if memory runs out. The caller
 * must free it. strdup() does the same, but it only became standard C in
 * C23; before that it is POSIX, and MSVC calls it _strdup. So the program
 * writes its own. */
static char *copy_string(const char *s)
{
    size_t size = strlen(s) + 1;                /* + 1 for the '\0' */
    char *copy = malloc(size);

    if (copy != NULL)
        memcpy(copy, s, size);
    return copy;
}

/* Adds task at the end of the list, growing the array if it is full: first
 * to FIRST_CAPACITY, then doubling (8, 16, 32...). Doubling keeps the
 * copying small, about one extra copy per task on average; growing by one
 * each time would copy the whole array on every add. The list takes
 * ownership of task.text. Returns 0 if memory runs out; the caller then
 * still owns task.text and must free it. */
static int list_append(TaskList *list, Task task)
{
    if (list->count == list->capacity) {
        if (list->capacity > INT_MAX / 2)
            return 0;                           /* doubling would overflow an int */
        int new_capacity = (list->capacity == 0) ? FIRST_CAPACITY : list->capacity * 2;
        /* realloc(NULL, size) works like malloc(size), so the first time needs
         * no special case. The result goes into a temporary pointer: if realloc
         * fails it returns NULL but the old array is still valid, and writing
         * items = realloc(items, ...) would lose the only pointer to it. */
        Task *bigger = realloc(list->items, (size_t)new_capacity * sizeof *bigger);
        if (bigger == NULL)
            return 0;
        list->items = bigger;
        list->capacity = new_capacity;
    }
    list->items[list->count++] = task;
    return 1;
}

/* Removes the task at index: frees its text, then moves every later task
 * down one place with memmove. The order must be kept, because positions
 * are the task numbers; the Birthday Reminder could fill the gap with the
 * last element only because its order did not matter. */
static void list_remove(TaskList *list, int index)
{
    free(list->items[index].text);
    memmove(&list->items[index], &list->items[index + 1],
            (size_t)(list->count - index - 1) * sizeof list->items[0]);
    list->count--;
}

/* Removes every completed task in one pass: a read index and a write index
 * walk the array together. A done task's text is freed and skipped; the
 * others are copied down. That is one pass, not one memmove per removed
 * task. The capacity does not shrink: realloc could shrink it, but for a
 * small list that is rarely worth it. Returns how many were removed. */
static int list_remove_done(TaskList *list)
{
    int write = 0, removed = 0;

    for (int read = 0; read < list->count; read++) {
        if (list->items[read].done) {
            free(list->items[read].text);
            removed++;
        } else {
            list->items[write++] = list->items[read];
        }
    }
    list->count = write;
    return removed;
}

/* Frees every task's text and the array, and empties the list. Every path
 * that ends the program calls this first. */
static void list_free(TaskList *list)
{
    for (int i = 0; i < list->count; i++)
        free(list->items[i].text);
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

/* qsort() comparison for an array of Task pointers: tasks still to do
 * first, then priority 1, 2, 3, then due date (none last), then position. */
static int compare_tasks(const void *a, const void *b)
{
    /* Each argument points to an element of the array, which is a Task *,
     * so a is really a "pointer to a const pointer to a const Task". */
    const Task *x = *(const Task *const *)a;
    const Task *y = *(const Task *const *)b;

    if (x->done != y->done)
        return x->done ? 1 : -1;
    if (x->priority != y->priority)
        return x->priority - y->priority;
    if ((x->due[0] == '\0') != (y->due[0] == '\0'))
        return x->due[0] == '\0' ? 1 : -1;     /* no due date goes last */
    int by_date = strcmp(x->due, y->due);       /* YYYY-MM-DD sorts as text */
    if (by_date != 0)
        return by_date;
    /* qsort is not stable, so finish with the position. Both pointers point
     * into the same array, so comparing them with < is well defined. */
    return (x < y) ? -1 : (x > y);
}

/* Prints the list in compare_tasks() order, with each task's position as
 * its number. The tasks themselves are not moved: a malloc'ed array of
 * pointers to them (a "view") is sorted instead, printed, then freed. */
static void show_list(const TaskList *list, const char *today)
{
    int done = 0;

    if (list->count == 0) {
        printf("Your to-do list is empty.\n");
        return;
    }
    const Task **view = malloc((size_t)list->count * sizeof *view);
    if (view == NULL) {
        printf("Out of memory.\n");
        return;
    }
    for (int i = 0; i < list->count; i++) {
        view[i] = &list->items[i];
        done += list->items[i].done;
    }
    qsort(view, (size_t)list->count, sizeof *view, compare_tasks);

    printf("To-do list: %d task%s, %d done (room for %d)\n",
           list->count, list->count == 1 ? "" : "s", done, list->capacity);
    printf("%3s  %-4s  %-8s  %-10s  %s\n", "#", "Done", "Priority", "Due", "Task");
    for (int i = 0; i < list->count; i++) {
        const Task *t = view[i];
        const char *note = "";
        /* YYYY-MM-DD dates compare correctly with strcmp: the year comes
         * first, then the month, then the day, all with leading zeros. */
        if (!t->done && t->due[0] != '\0' && strcmp(t->due, today) < 0)
            note = "  (overdue)";
        else if (!t->done && strcmp(t->due, today) == 0)
            note = "  (due today)";
        printf("%3d  %-4s  %-8s  %-10s  %s%s\n", (int)(t - list->items) + 1,
               t->done ? "[x]" : "[ ]", priority_names[t->priority],
               t->due[0] ? t->due : "-", t->text, note);
    }
    free(view);
}

/* Reads one line of any length from f into a malloc'ed string stored in
 * *line, without the '\n' (or a '\r' before it, from a file written on
 * Windows). The buffer starts at FIRST_LINE bytes, small on purpose so
 * that ordinary lines already make it grow, and doubles whenever it fills.
 * The caller frees *line. Returns 1 for a line, 0 when the file ends before
 * any character is read, -1 if memory runs out. */
static int read_file_line(FILE *f, char **line)
{
    size_t size = FIRST_LINE, len = 0;
    char *buf = malloc(size);
    int c;

    if (buf == NULL)
        return -1;
    while ((c = getc(f)) != EOF && c != '\n') {
        if (len + 1 == size) {                  /* full: keep room for the '\0' */
            char *bigger = realloc(buf, size * 2);
            if (bigger == NULL) {
                free(buf);
                return -1;
            }
            buf = bigger;
            size *= 2;
        }
        buf[len++] = (char)c;
    }
    if (c == EOF && len == 0) {
        free(buf);
        return 0;                               /* nothing left to read */
    }
    if (len > 0 && buf[len - 1] == '\r')
        len--;
    buf[len] = '\0';
    *line = buf;
    return 1;
}

/* Checks a file line has the shape done|priority|due|text: 0 or 1, then
 * 1, 2 or 3, then - or a real YYYY-MM-DD date, then at least one character
 * of text, which may itself contain |. Fills *out, with out->text a
 * malloc'ed copy. Returns 1 if valid, 0 if not, -1 if memory runs out. */
static int parse_task_line(const char *line, Task *out)
{
    const char *rest, *text;

    /* Each test only runs if the one before passed, so no character past
     * the end of the string is ever read. */
    if ((line[0] != '0' && line[0] != '1') || line[1] != '|' ||
        line[2] < '1' || line[2] > '3' || line[3] != '|')
        return 0;
    rest = line + 4;
    if (rest[0] == '-' && rest[1] == '|') {
        out->due[0] = '\0';
        text = rest + 2;
    } else {
        if (strlen(rest) < 11 || rest[10] != '|')
            return 0;
        memcpy(out->due, rest, 10);
        out->due[10] = '\0';
        if (!is_valid_date(out->due))
            return 0;
        text = rest + 11;
    }
    if (text[0] == '\0')
        return 0;
    out->done = (line[0] == '1');
    out->priority = line[2] - '0';
    out->text = copy_string(text);
    return out->text != NULL ? 1 : -1;
}

/* Loads the tasks from the file at path. A missing file means an empty
 * list. Lines that are not valid tasks are skipped and counted; a blank
 * line is ignored. Running out of memory is fatal: everything is freed.
 * Returns 0 if the file cannot be opened or memory runs out. */
static int load_list(TaskList *list, const char *path)
{
    int skipped = 0, result = 0;
    char *line = NULL;
    FILE *f = fopen(path, "r");

    if (f == NULL) {
        if (errno == ENOENT) {
            printf("No file %s yet: starting a new list.\n", path);
            return 1;
        }
        printf("Could not open %s: %s\n", path, strerror(errno));
        return 0;
    }
    while ((result = read_file_line(f, &line)) == 1) {
        Task task = { NULL, 0, "", false };
        int parsed = (line[0] == '\0') ? 2 : parse_task_line(line, &task);
        free(line);
        if (parsed == 0)
            skipped++;
        if (parsed < 0 || (parsed == 1 && !list_append(list, task))) {
            if (parsed == 1)
                free(task.text);                /* list_append did not take it */
            result = -1;
            break;
        }
    }
    fclose(f);                                  /* only read from: nothing to lose */
    if (result < 0) {
        list_free(list);
        printf("Out of memory.\n");
        return 0;
    }
    printf("Loaded %d task%s from %s.\n", list->count, list->count == 1 ? "" : "s", path);
    if (skipped > 0)
        printf("(%d line%s skipped: not valid task lines)\n", skipped, skipped == 1 ? "" : "s");
    return 1;
}

/* Saves every task to path the safe way, as the address book does: write
 * path.tmp, close it and check for errors, then rename the new file over
 * the old one (removing the old one first only where rename requires it, as
 * on Windows). Prints nothing on success. Returns 1 on success; otherwise
 * prints why and returns 0. */
static int save_list(const TaskList *list, const char *path)
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
        const Task *t = &list->items[i];
        fprintf(f, "%d|%d|%s|%s\n", t->done, t->priority, t->due[0] ? t->due : "-", t->text);
    }
    int failed = ferror(f);
    if (fclose(f) != 0 || failed) {
        printf("Could not save: %s\n", strerror(errno));
        remove(tmp);
        return 0;
    }
    /* rename() replaces the old file in one step on Linux and macOS. On
     * Windows it fails if the old file exists, so remove that and try
     * again. If the second try fails too, FILE.tmp is now the only copy
     * of the data, so it is kept, never deleted. */
    if (rename(tmp, path) != 0) {
        remove(path);
        if (rename(tmp, path) != 0) {
            printf("Could not save: %s. Your tasks are in %s.\n", strerror(errno), tmp);
            return 0;
        }
    }
    return 1;
}

/* Reads a due date into due: a real YYYY-MM-DD date, or an empty line for
 * none. A date in the past is allowed: the task is simply overdue.
 * Returns 1 on success, 0 at end of input. */
static int read_due_date(char due[DATE_SIZE])
{
    char buf[LINE_SIZE];

    for (;;) {
        printf("Due date (YYYY-MM-DD, or press Enter for none): ");
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;
        if (buf[0] == '\0') {
            due[0] = '\0';
            return 1;
        }
        if (is_valid_date(buf)) {
            strcpy(due, buf);
            return 1;
        }
        printf("Please enter a date as YYYY-MM-DD, or press Enter for none.\n");
    }
}

/* Asks for a task's text, priority and due date, adds it and saves the
 * file. Returns 0 if input ended, else 1. */
static int add_task(TaskList *list, const char *path)
{
    char text[TEXT_SIZE];
    Task task;

    if (!read_text("Task: ", text, TEXT_SIZE) ||
        !read_int("Priority (1 high, 2 medium, 3 low): ", 1, 3, &task.priority) ||
        !read_due_date(task.due))
        return 0;
    task.done = false;
    task.text = copy_string(text);
    if (task.text == NULL || !list_append(list, task)) {
        free(task.text);                        /* free(NULL) does nothing */
        printf("Out of memory: the task was not added.\n");
        return 1;
    }
    printf("Added task %d.\n", list->count);
    save_list(list, path);
    return 1;
}

/* Asks for a task number and marks that task as done, then saves the file.
 * Returns 0 if input ended, else 1. */
static int mark_done(TaskList *list, const char *path)
{
    int n = 0;

    if (list->count == 0) {
        printf("Your to-do list is empty.\n");
        return 1;
    }
    if (!read_int("Mark which task as done? (0 to cancel): ", 0, list->count, &n))
        return 0;
    if (n == 0) {
        printf("Nothing changed.\n");
    } else if (list->items[n - 1].done) {
        printf("Task %d is already done.\n", n);
    } else {
        list->items[n - 1].done = true;
        printf("Marked task %d as done: %s\n", n, list->items[n - 1].text);
        save_list(list, path);
    }
    return 1;
}

/* Asks for a task number and deletes that task, then saves the file.
 * Returns 0 if input ended, else 1. */
static int delete_task(TaskList *list, const char *path)
{
    int n = 0;

    if (list->count == 0) {
        printf("Your to-do list is empty.\n");
        return 1;
    }
    if (!read_int("Delete which task? (0 to cancel): ", 0, list->count, &n))
        return 0;
    if (n == 0) {
        printf("Nothing deleted.\n");
        return 1;
    }
    /* Print first: list_remove() frees the text, and using it after that
     * would read memory that no longer belongs to the program. */
    printf("Deleted task %d: %s\n", n, list->items[n - 1].text);
    list_remove(list, n - 1);
    save_list(list, path);
    return 1;
}
