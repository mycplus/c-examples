/*
 * text_editor.c - a line editor, in the tradition of Unix ed.
 *
 * You work on a file with short commands typed at the edit> prompt:
 * p prints lines, a adds lines, c changes one, d deletes, f finds,
 * r replaces text, w writes the file and q quits. There are no cursor keys
 * and no screen control, so it is standard C and runs anywhere. Lines can
 * be any length, and indentation is kept exactly. The prompt becomes
 * edit*> while there are unsaved changes.
 *
 * Reused from earlier projects, copied unchanged:
 *   read_line                               unit_converter.c  (project 1)
 *   contains_ignore_case                    personal_diary.c  (project 10)
 *   ask_yes_no                              address_book.c    (project 11)
 *   copy_string, read_file_line             todo_organizer.c  (project 13)
 *
 * Build with GCC or Clang:
 *     gcc -std=c11 -Wall -Wextra -o text_editor text_editor.c
 * Build with Visual Studio (Developer Command Prompt):
 *     cl /W4 text_editor.c
 * Run:
 *     ./text_editor notes.txt
 */
#define _CRT_SECURE_NO_WARNINGS   /* fopen, strerror, remove, rename on MSVC */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE      128        /* read_line buffer, for commands and answers only */
#define PATH_SIZE      260
#define FIRST_CAPACITY 16         /* first allocation of the lines array */
#define FIRST_LINE     16         /* read_file_line's first buffer: small on purpose */

/* The text being edited: an array of pointers to strings, one malloc'ed
 * string per line. The buffer owns the array and every line. */
typedef struct {
    char **lines;                 /* malloc'ed array of malloc'ed strings */
    int    count;                 /* lines in use */
    int    capacity;              /* room in the array */
    bool   changed;               /* unsaved changes? */
    char   path[PATH_SIZE];       /* the file being edited */
} Buffer;

static int   read_line(char *buf, size_t size);
static int   contains_ignore_case(const char *text, const char *word);
static int   ask_yes_no(const char *prompt);
static char *copy_string(const char *s);
static int   read_file_line(FILE *f, char **line);
static int   insert_line(Buffer *buf, int index, char *text);
static void  delete_lines(Buffer *buf, int first, int last);
static void  free_buffer(Buffer *buf);
static int   load_file(Buffer *buf, const char *path);
static int   save_file(Buffer *buf, const char *path);
static char *replace_all(const char *text, const char *find, const char *with, int *count);
static int   parse_command(char *line, char *cmd, int nums[2], int *how_many, char **word);
static int   check_range(const Buffer *buf, int first, int last);
static void  print_lines(const Buffer *buf, int first, int last);
static int   cmd_add(Buffer *buf, int after);
static int   cmd_change(Buffer *buf, int n);
static int   cmd_replace(Buffer *buf);
static void  cmd_find(const Buffer *buf, const char *word);
static void  show_help(void);

int main(int argc, char *argv[])
{
    Buffer buf = { NULL, 0, 0, false, "" };
    char line[LINE_SIZE], prompt[PATH_SIZE + 40], cmd = 0, *word = NULL;
    int nums[2] = { 0, 0 }, how_many = 0, ok = 1;

    if (argc != 2 || strlen(argv[1]) >= PATH_SIZE - 4) {
        fprintf(stderr, "Usage: text_editor FILE\n");
        return 1;
    }
    strcpy(buf.path, argv[1]);

    printf("Text Editor\n===========\n");
    int loaded = load_file(&buf, buf.path);
    if (loaded < 0)
        return 1;                               /* load_file printed why and freed everything */
    if (loaded == 0)
        printf("Editing %s (new file).\n", buf.path);
    else
        printf("Editing %s: %d line%s.\n", buf.path, buf.count, buf.count == 1 ? "" : "s");
    printf("Type h for help.\n");

    while (ok) {
        printf("%s", buf.changed ? "edit*> " : "edit> ");
        fflush(stdout);
        if (!read_line(line, sizeof line)) {
            ok = 0;                             /* input ended */
            break;
        }
        if (!parse_command(line, &cmd, nums, &how_many, &word))
            cmd = '?';
        int n = nums[0], m = (how_many == 2) ? nums[1] : nums[0];

        if (cmd == 'p' && how_many == 0) {
            if (buf.count == 0)
                printf("The file is empty.\n");
            else
                print_lines(&buf, 1, buf.count);
        } else if (cmd == 'p') {
            if (check_range(&buf, n, m))
                print_lines(&buf, n, m);
        } else if (cmd == 'a' && how_many <= 1) {
            int after = (how_many == 0) ? buf.count : n;
            if (after > buf.count)
                printf("Line numbers go from 0 to %d.\n", buf.count);
            else
                ok = cmd_add(&buf, after);
        } else if (cmd == 'c' && how_many == 1) {
            ok = cmd_change(&buf, n);
        } else if (cmd == 'd' && how_many >= 1) {
            if (check_range(&buf, n, m)) {
                delete_lines(&buf, n - 1, m - 1);
                printf("Deleted %d line%s.\n", m - n + 1, m == n ? "" : "s");
            }
        } else if (cmd == 'f') {
            cmd_find(&buf, word);
        } else if (cmd == 'r' && how_many == 0) {
            ok = cmd_replace(&buf);
        } else if (cmd == 'w') {
            if (strlen(word) >= PATH_SIZE - 4)
                printf("Could not save: the file name is too long.\n");
            else
                save_file(&buf, word[0] != '\0' ? word : buf.path);
        } else if (cmd == 'q' && how_many == 0) {
            if (!buf.changed)
                break;
            snprintf(prompt, sizeof prompt, "Save changes to %s? (y/n): ", buf.path);
            int answer = ask_yes_no(prompt);
            if (answer < 0)
                ok = 0;                         /* input ended */
            else if (answer == 0) {
                printf("Changes discarded.\n");
                break;
            } else if (save_file(&buf, buf.path))
                break;                          /* otherwise stay in the editor */
        } else if (cmd == 'h' && how_many == 0) {
            show_help();
        } else {
            printf("Unknown command. Type h for help.\n");
        }
    }
    /* End of input (ok is 0) leaves without saving, as when an editor is
     * closed without saving: unsaved changes are lost. */
    fputs(ok ? "\nGoodbye!\n" : "\n\nGoodbye!\n", stdout);
    free_buffer(&buf);
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

/* Returns 1 if word appears anywhere in text, ignoring upper and lower case.
 * strcasestr() and stristr() would do this, but they are not standard C:
 * for each starting place in text, compare the letters of word one by one. */
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

/* Prints prompt and reads y, yes, n or no in any case, with spaces around
 * it allowed. Anything else asks again.
 * Returns 1 for yes, 0 for no, -1 at end of input. */
static int ask_yes_no(const char *prompt)
{
    char buf[LINE_SIZE];

    for (;;) {
        char *p = buf, *end;

        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return -1;
        while (*p == ' ' || *p == '\t')
            p++;
        end = p + strlen(p);
        while (end > p && (end[-1] == ' ' || end[-1] == '\t'))
            end--;
        *end = '\0';                            /* cut the trailing spaces */
        for (char *q = p; *q != '\0'; q++)
            *q = (char)tolower((unsigned char)*q);
        if (strcmp(p, "y") == 0 || strcmp(p, "yes") == 0)
            return 1;
        if (strcmp(p, "n") == 0 || strcmp(p, "no") == 0)
            return 0;
        printf("Please answer y or n.\n");
    }
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

/* Inserts text as line index (0-based), moving the later lines down. The
 * buffer takes ownership of text. The array grows like the to-do list's:
 * FIRST_CAPACITY, then doubling, with realloc into a temporary pointer so
 * the old array is not lost if it fails. memmove shifts only the pointers,
 * 8 bytes each, however long the lines are: that is why editors keep text
 * as an array of lines. Returns 0 if memory runs out; the caller then
 * still owns text. */
static int insert_line(Buffer *buf, int index, char *text)
{
    if (buf->count == buf->capacity) {
        if (buf->capacity > INT_MAX / 2)
            return 0;
        int new_capacity = (buf->capacity == 0) ? FIRST_CAPACITY : buf->capacity * 2;
        char **bigger = realloc(buf->lines, (size_t)new_capacity * sizeof *bigger);
        if (bigger == NULL)
            return 0;
        buf->lines = bigger;
        buf->capacity = new_capacity;
    }
    memmove(&buf->lines[index + 1], &buf->lines[index],
            (size_t)(buf->count - index) * sizeof buf->lines[0]);
    buf->lines[index] = text;
    buf->count++;
    return 1;
}

/* Deletes lines first to last (0-based, inclusive): frees each one, then
 * moves the later pointers down over the gap. Again only pointers move. */
static void delete_lines(Buffer *buf, int first, int last)
{
    for (int i = first; i <= last; i++)
        free(buf->lines[i]);
    memmove(&buf->lines[first], &buf->lines[last + 1],
            (size_t)(buf->count - last - 1) * sizeof buf->lines[0]);
    buf->count -= last - first + 1;
    buf->changed = true;
}

/* Frees every line and the array, and empties the buffer. Every way out of
 * the program calls this, so each line is freed exactly once. */
static void free_buffer(Buffer *buf)
{
    for (int i = 0; i < buf->count; i++)
        free(buf->lines[i]);
    free(buf->lines);
    buf->lines = NULL;
    buf->count = 0;
    buf->capacity = 0;
}

/* Reads the file at path into the buffer, one line of any length at a time.
 * read_file_line() doubles its buffer as it reads, so a line can sit in up
 * to twice the memory it needs; the buffer keeps an exact-size copy made by
 * copy_string() instead, which matters for a file of many lines.
 * Returns 1 if it was loaded, 0 if it does not exist yet (a new file), or
 * -1 if it cannot be opened or memory runs out (after printing why and
 * freeing everything). */
static int load_file(Buffer *buf, const char *path)
{
    char *text = NULL;
    int result;
    FILE *f = fopen(path, "r");

    if (f == NULL) {
        if (errno == ENOENT)
            return 0;
        printf("Could not open %s: %s\n", path, strerror(errno));
        return -1;
    }
    while ((result = read_file_line(f, &text)) == 1) {
        char *exact = copy_string(text);
        free(text);
        if (exact == NULL || !insert_line(buf, buf->count, exact)) {
            free(exact);                        /* free(NULL) does nothing */
            result = -1;
            break;
        }
    }
    fclose(f);                                  /* only read from: nothing to lose */
    if (result < 0) {
        free_buffer(buf);
        printf("Out of memory.\n");
        return -1;
    }
    return 1;
}

/* Writes every line, each followed by '\n', to path the safe way: write
 * path.tmp, close it, then rename the new file over the old one (removing
 * the old one first only where rename requires it, as on Windows). On
 * success path becomes the buffer's file (so "w NAME" is "save as") and
 * the changed flag is cleared. Returns 1 on success, else prints why and 0. */
static int save_file(Buffer *buf, const char *path)
{
    char tmp[PATH_SIZE + 4];
    FILE *f;

    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    f = fopen(tmp, "w");
    if (f == NULL) {
        printf("Could not save: %s\n", strerror(errno));
        return 0;
    }
    for (int i = 0; i < buf->count; i++)
        fprintf(f, "%s\n", buf->lines[i]);
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
            printf("Could not save: %s. Your lines are in %s.\n", strerror(errno), tmp);
            return 0;
        }
    }
    if (path != buf->path)
        strcpy(buf->path, path);                /* save as: later w commands use it */
    buf->changed = false;
    printf("Wrote %d line%s to %s.\n", buf->count, buf->count == 1 ? "" : "s", buf->path);
    return 1;
}

/* Returns a new malloc'ed copy of text with every find replaced by with,
 * and sets *count to the number of replacements. Matches are found left to
 * right and do not overlap, so "aa" -> "b" in "aaa" gives "ba". It takes
 * two passes because the new size must be known before the malloc: first
 * count the matches, then copy the pieces. Returns NULL without allocating
 * if *count is 0, or NULL with *count > 0 if memory runs out. find must
 * not be empty. */
static char *replace_all(const char *text, const char *find, const char *with, int *count)
{
    size_t find_len = strlen(find), with_len = strlen(with), len = strlen(text);
    int n = 0;

    for (const char *p = strstr(text, find); p != NULL; p = strstr(p + find_len, find))
        n++;
    *count = n;
    if (n == 0)
        return NULL;
    /* size_t cannot go negative, so a shorter replacement is a subtraction. */
    size_t new_len = (with_len >= find_len) ? len + (size_t)n * (with_len - find_len)
                                            : len - (size_t)n * (find_len - with_len);
    char *out = malloc(new_len + 1);
    if (out == NULL)
        return NULL;

    char *q = out;
    const char *s = text, *p;
    while ((p = strstr(s, find)) != NULL) {
        memcpy(q, s, (size_t)(p - s));          /* the text before the match */
        q += p - s;
        memcpy(q, with, with_len);              /* the replacement */
        q += with_len;
        s = p + find_len;
    }
    strcpy(q, s);                               /* the rest, with its '\0' */
    return out;
}

/* Splits a command line (trimmed in place) into a command letter and,
 * for f and w, *word: the rest of the line after spaces. For every other
 * command, up to two whole numbers may follow, read with strtol and checked
 * against INT_MAX; *how_many says how many. Returns 1 if the line has that
 * shape, 0 otherwise (an empty line, a letter after a number, a third
 * number, -1...). */
static int parse_command(char *line, char *cmd, int nums[2], int *how_many, char **word)
{
    char *p = line, *end = line + strlen(line);

    while (*p == ' ' || *p == '\t')
        p++;
    while (end > p && (end[-1] == ' ' || end[-1] == '\t'))
        end--;
    *end = '\0';
    if (*p == '\0')
        return 0;
    *cmd = *p++;
    *how_many = 0;
    *word = p;
    while (**word == ' ' || **word == '\t')
        (*word)++;
    if (*cmd == 'f' || *cmd == 'w')
        return 1;
    for (;;) {
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '\0')
            return 1;
        if (!isdigit((unsigned char)*p) || *how_many == 2)
            return 0;
        errno = 0;
        long value = strtol(p, &end, 10);
        if (errno == ERANGE || value > INT_MAX || (*end != '\0' && *end != ' ' && *end != '\t'))
            return 0;
        nums[(*how_many)++] = (int)value;
        p = end;
    }
}

/* Checks that lines first to last (1-based) exist and are in order.
 * Returns 1 if they do; otherwise prints why and returns 0. */
static int check_range(const Buffer *buf, int first, int last)
{
    if (buf->count == 0) {
        printf("The file is empty.\n");
        return 0;
    }
    if (first < 1 || last > buf->count || first > last) {
        printf("Line numbers go from 1 to %d.\n", buf->count);
        return 0;
    }
    return 1;
}

/* Prints lines first to last (1-based), each with its number. */
static void print_lines(const Buffer *buf, int first, int last)
{
    for (int n = first; n <= last; n++)
        printf("%4d  %s\n", n, buf->lines[n - 1]);
}

/* Reads lines and adds them after line after (0 = at the top) until a line
 * that is just "." or the end of input. Lines come from read_file_line(),
 * the same function that reads the file: stdin is a FILE * too. read_line
 * would empty a line that is too long, and read_text would trim the spaces
 * that make up indentation; an editor must keep text exactly.
 * Returns 1: the end of input only ends the adding. */
static int cmd_add(Buffer *buf, int after)
{
    char *text = NULL;
    int added = 0, result;

    printf("Adding after line %d. Type a line with just . to stop.\n", after);
    while ((result = read_file_line(stdin, &text)) == 1) {
        if (strcmp(text, ".") == 0) {
            free(text);
            break;
        }
        char *exact = copy_string(text);        /* exact size, as in load_file() */
        free(text);
        if (exact == NULL || !insert_line(buf, after + added, exact)) {
            free(exact);
            result = -1;
            break;
        }
        added++;
    }
    if (result < 0)
        printf("Out of memory.\n");
    if (added > 0)
        buf->changed = true;
    printf("Added %d line%s.\n", added, added == 1 ? "" : "s");
    return 1;
}

/* Shows line n and replaces it with a new line typed in full (any length,
 * spaces kept). The old text is freed. Returns 0 if input ended, else 1. */
static int cmd_change(Buffer *buf, int n)
{
    char *text = NULL;

    if (!check_range(buf, n, n))
        return 1;
    printf("Line %d: %s\n", n, buf->lines[n - 1]);
    printf("New text: ");
    fflush(stdout);
    int result = read_file_line(stdin, &text);
    if (result == 0)
        return 0;
    char *exact = (result > 0) ? copy_string(text) : NULL;   /* exact size, as in load_file() */
    free(text);
    if (exact == NULL) {
        printf("Out of memory.\n");
        return 1;
    }
    free(buf->lines[n - 1]);
    buf->lines[n - 1] = exact;
    buf->changed = true;
    printf("Changed line %d.\n", n);
    return 1;
}

/* Asks what to find and what to put instead, then replaces every exact,
 * case-sensitive match in every line. Each changed line is a new string;
 * the old one is freed. Returns 0 if input ended, else 1. */
static int cmd_replace(Buffer *buf)
{
    char find[LINE_SIZE], with[LINE_SIZE];
    int total = 0, lines_changed = 0;

    printf("Find: ");
    fflush(stdout);
    if (!read_line(find, sizeof find))
        return 0;
    if (find[0] == '\0') {
        printf("Nothing to find.\n");
        return 1;
    }
    printf("Replace with: ");
    fflush(stdout);
    if (!read_line(with, sizeof with))
        return 0;
    for (int i = 0; i < buf->count; i++) {
        int n = 0;
        char *changed = replace_all(buf->lines[i], find, with, &n);
        if (n == 0)
            continue;
        if (changed == NULL) {
            printf("Out of memory.\n");
            break;
        }
        free(buf->lines[i]);
        buf->lines[i] = changed;
        total += n;
        lines_changed++;
    }
    if (total == 0) {
        printf("No line contains \"%s\".\n", find);
        return 1;
    }
    buf->changed = true;
    printf("Replaced %d occurrence%s in %d line%s.\n", total, total == 1 ? "" : "s",
           lines_changed, lines_changed == 1 ? "" : "s");
    return 1;
}

/* Prints every line containing word, ignoring case, then how many did. */
static void cmd_find(const Buffer *buf, const char *word)
{
    int found = 0;

    if (word[0] == '\0') {
        printf("Usage: f WORD\n");
        return;
    }
    for (int i = 0; i < buf->count; i++) {
        if (contains_ignore_case(buf->lines[i], word)) {
            printf("%4d  %s\n", i + 1, buf->lines[i]);
            found++;
        }
    }
    if (found == 0)
        printf("No line contains \"%s\".\n", word);
    else
        printf("%d line%s \"%s\".\n", found, found == 1 ? " contains" : "s contain", word);
}

/* Prints the list of commands. */
static void show_help(void)
{
    printf("Commands (N and M are line numbers):\n"
           "  p [N [M]]   print all lines, line N, or lines N to M\n"
           "  a [N]       add lines after line N (0 = at the top; default: at the end)\n"
           "  c N         change line N\n"
           "  d N [M]     delete line N, or lines N to M\n"
           "  f WORD      find lines containing WORD (ignoring case)\n"
           "  r           replace text in every line (exact match)\n"
           "  w [NAME]    write the file, or save it as NAME\n"
           "  q           quit\n"
           "  h           show this help\n");
}
