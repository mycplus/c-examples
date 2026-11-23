/*
 * unit_converter.c - a menu-driven unit converter for length, mass,
 * temperature and volume.
 *
 * Pick a category, a "from" unit, a "to" unit and a value; the program
 * prints the converted value and returns to the menu. Choose 0 to quit.
 *
 * Build with GCC or Clang:
 *     gcc -std=c11 -Wall -Wextra -o unit_converter unit_converter.c
 * Build with Visual Studio (Developer Command Prompt):
 *     cl /W4 unit_converter.c
 * Run:
 *     ./unit_converter          (Linux, macOS, WSL)
 *     unit_converter.exe        (Windows)
 */
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 64
#define ABSOLUTE_ZERO_C (-273.15)

/* One unit: its name, its symbol and its size in the category's base unit. */
typedef struct {
    const char *name;    /* "kilometre" */
    const char *symbol;  /* "km"        */
    double to_base;      /* size of one of this unit in the category's base unit */
} Unit;

/* One category: a menu name, a word for error messages and its unit table. */
typedef struct {
    const char *name;      /* "Length", shown in menus */
    const char *quantity;  /* "length", used in error messages */
    const Unit *units;
    int count;
    int is_temperature;    /* 1: use the temperature formulas, ignore to_base */
} Category;

/* Temperature units, in the same order as temperature_units[] below. */
enum { TEMP_C, TEMP_F, TEMP_K };

/* Base unit: metre. All factors are exact by definition (NIST SP 811). */
static const Unit length_units[] = {
    { "millimetre", "mm", 0.001 },
    { "centimetre", "cm", 0.01 },
    { "metre",      "m",  1.0 },
    { "kilometre",  "km", 1000.0 },
    { "inch",       "in", 0.0254 },
    { "foot",       "ft", 0.3048 },
    { "yard",       "yd", 0.9144 },
    { "mile",       "mi", 1609.344 },
};

/* Base unit: kilogram. */
static const Unit mass_units[] = {
    { "milligram", "mg", 0.000001 },
    { "gram",      "g",  0.001 },
    { "kilogram",  "kg", 1.0 },
    { "tonne",     "t",  1000.0 },
    { "ounce",     "oz", 0.028349523125 },
    { "pound",     "lb", 0.45359237 },
};

/* Temperature does not use to_base: the scales have different zero points. */
static const Unit temperature_units[] = {
    { "Celsius",    "C", 0.0 },
    { "Fahrenheit", "F", 0.0 },
    { "Kelvin",     "K", 0.0 },
};

/* Base unit: litre. */
static const Unit volume_units[] = {
    { "millilitre",      "mL",       0.001 },
    { "litre",           "L",        1.0 },
    { "cubic metre",     "m3",       1000.0 },
    { "US fluid ounce",  "US fl oz", 0.0295735295625 },
    { "US gallon",       "US gal",   3.785411784 },
    { "imperial gallon", "imp gal",  4.54609 },
};

#define COUNT(table) ((int)(sizeof table / sizeof table[0]))

static const Category categories[] = {
    { "Length",      "length",      length_units,      COUNT(length_units),      0 },
    { "Mass",        "mass",        mass_units,        COUNT(mass_units),        0 },
    { "Temperature", "temperature", temperature_units, COUNT(temperature_units), 1 },
    { "Volume",      "volume",      volume_units,      COUNT(volume_units),      0 },
};

static int    read_line(char *buf, size_t size);
static int    read_int(const char *prompt, int lo, int hi, int *value);
static int    read_double(const char *prompt, double *value);
static void   show_menu(const Category cats[], int count);
static void   list_units(const Category *cat);
static int    run_category(const Category *cat);
static double convert_linear(double value, const Unit *from, const Unit *to);
static double to_celsius(double value, int unit);
static double from_celsius(double celsius, int unit);

int main(void)
{
    const int count = COUNT(categories);
    char prompt[LINE_SIZE];
    int choice = 0;

    snprintf(prompt, sizeof prompt, "Choose a category (0-%d): ", count);
    printf("Unit Converter\n==============\n");

    for (;;) {
        show_menu(categories, count);
        if (!read_int(prompt, 0, count, &choice))
            break;                              /* input ended */
        if (choice == 0) {
            printf("\nGoodbye!\n");
            return 0;
        }
        if (!run_category(&categories[choice - 1]))
            break;                              /* input ended */
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

/* Prints a blank line, the numbered categories and 0 to quit. */
static void show_menu(const Category cats[], int count)
{
    printf("\n");
    for (int i = 0; i < count; i++)
        printf("%d. %s\n", i + 1, cats[i].name);
    printf("0. Quit\n");
}

/* Prints a blank line, the category heading and its numbered units. */
static void list_units(const Category *cat)
{
    printf("\n%s units:\n", cat->name);
    for (int i = 0; i < cat->count; i++)
        printf("  %d. %s (%s)\n", i + 1, cat->units[i].name, cat->units[i].symbol);
}

/* Runs one conversion: asks for two units and a value, checks the value
 * against physical limits and prints the result. Lengths, masses and volumes
 * cannot be negative; a temperature cannot be below absolute zero.
 * Returns 1 when done, 0 if input ended. */
static int run_category(const Category *cat)
{
    char from_prompt[LINE_SIZE], to_prompt[LINE_SIZE], value_prompt[LINE_SIZE];
    int from = 0, to = 0;
    double value = 0.0, result = 0.0;
    const Unit *u_from, *u_to;

    list_units(cat);
    snprintf(from_prompt, sizeof from_prompt, "Convert from (1-%d): ", cat->count);
    snprintf(to_prompt, sizeof to_prompt, "Convert to (1-%d): ", cat->count);
    if (!read_int(from_prompt, 1, cat->count, &from) ||
        !read_int(to_prompt, 1, cat->count, &to))
        return 0;
    u_from = &cat->units[from - 1];
    u_to = &cat->units[to - 1];
    snprintf(value_prompt, sizeof value_prompt, "Value in %s: ", u_from->symbol);

    for (;;) {
        if (!read_double(value_prompt, &value))
            return 0;
        if (cat->is_temperature) {
            double celsius = to_celsius(value, from - 1);
            if (celsius < ABSOLUTE_ZERO_C) {
                printf("That is below absolute zero (-273.15 C, -459.67 F, 0 K).\n");
                continue;
            }
            result = from_celsius(celsius, to - 1);
        } else {
            if (value < 0.0) {
                printf("A %s cannot be negative.\n", cat->quantity);
                continue;
            }
            result = convert_linear(value, u_from, u_to);
        }
        break;
    }
    printf("%.6g %s = %.6g %s\n", value, u_from->symbol, result, u_to->symbol);
    return 1;
}

/* Converts value between two units of one category through the base unit:
 * multiply into the base, divide out of it. */
static double convert_linear(double value, const Unit *from, const Unit *to)
{
    return value * from->to_base / to->to_base;
}

/* Converts a temperature in unit (TEMP_C, TEMP_F or TEMP_K) to Celsius. */
static double to_celsius(double value, int unit)
{
    switch (unit) {
    case TEMP_F: return (value - 32.0) * 5.0 / 9.0;
    case TEMP_K: return value - 273.15;
    default:     return value;               /* already Celsius */
    }
}

/* Converts a temperature in Celsius to unit (TEMP_C, TEMP_F or TEMP_K). */
static double from_celsius(double celsius, int unit)
{
    switch (unit) {
    case TEMP_F: return celsius * 9.0 / 5.0 + 32.0;
    case TEMP_K: return celsius + 273.15;
    default:     return celsius;             /* already Celsius */
    }
}
