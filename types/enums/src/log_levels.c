/* log_levels: an X-macro keeps an enum and its names in one list, so they
   cannot drift apart when a level is added.

   usage: log_levels [debug|info|warning|error]   (the minimum level to show) */
#include <stdio.h>
#include <string.h>

#define LOG_LEVELS(X) \
    X(DEBUG, "debug")  \
    X(INFO, "info")    \
    X(WARNING, "warning") \
    X(ERROR, "error")

enum log_level {
#define AS_ENUM(name, text) LOG_##name,
    LOG_LEVELS(AS_ENUM)
#undef AS_ENUM
    LOG_LEVEL_COUNT
};

static const char *const log_level_names[] = {
#define AS_NAME(name, text) text,
    LOG_LEVELS(AS_NAME)
#undef AS_NAME
};

_Static_assert(sizeof log_level_names / sizeof log_level_names[0] == LOG_LEVEL_COUNT,
               "every log level needs a name");

static void log_message(enum log_level minimum, enum log_level level, const char *text)
{
    if (level >= minimum)
        printf("[%-7s] %s\n", log_level_names[level], text);
}

int main(int argc, char **argv)
{
    enum log_level minimum = LOG_INFO;
    if (argc > 1) {
        int found = 0;
        for (int i = 0; i < LOG_LEVEL_COUNT; i++)
            if (strcmp(argv[1], log_level_names[i]) == 0) {
                minimum = (enum log_level)i;
                found = 1;
            }
        if (!found) {
            fprintf(stderr, "log_levels: unknown level \"%s\"\n", argv[1]);
            return 2;
        }
    }
    printf("%d levels; showing %s and above\n", LOG_LEVEL_COUNT, log_level_names[minimum]);
    log_message(minimum, LOG_DEBUG, "cache lookup for key 42");
    log_message(minimum, LOG_INFO, "server started on port 8080");
    log_message(minimum, LOG_WARNING, "disk 91% full");
    log_message(minimum, LOG_ERROR, "payment service unreachable");
    return 0;
}
