/* basics: declaring enums, the values the compiler assigns, explicit
   values, aliases, and looping over a contiguous enum. */
#include <stdio.h>

/* Values start at 0 and go up by one. */
enum weekday { MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY, SUNDAY };

/* Explicit values, as in HTTP status codes. A constant without "=" is one
   more than the constant before it. */
enum http_status {
    HTTP_OK = 200,
    HTTP_CREATED,                 /* 201 */
    HTTP_NOT_FOUND = 404,
    HTTP_SERVER_ERROR = 500,
};

/* Two names may share a value: here the last real level doubles as the
   number of levels, so the enum can size an array. */
enum fan_speed { FAN_OFF, FAN_LOW, FAN_MEDIUM, FAN_HIGH, FAN_SPEED_COUNT, FAN_MAX = FAN_HIGH };

int main(void)
{
    enum weekday today = WEDNESDAY;
    printf("WEDNESDAY = %d, SUNDAY = %d\n", (int)today, SUNDAY);
    printf("weekend? %s\n", today >= SATURDAY ? "yes" : "no");

    printf("HTTP_OK = %d, HTTP_CREATED = %d, HTTP_NOT_FOUND = %d\n", HTTP_OK, HTTP_CREATED, HTTP_NOT_FOUND);

    static const char *const fan_names[FAN_SPEED_COUNT] = {"off", "low", "medium", "high"};
    for (enum fan_speed s = FAN_OFF; s < FAN_SPEED_COUNT; s++)
        printf("fan %d: %s%s\n", (int)s, fan_names[s], s == FAN_MAX ? " (max)" : "");

    printf("sizeof(enum weekday) = %zu, sizeof(int) = %zu\n", sizeof(enum weekday), sizeof(int));
    return 0;
}
