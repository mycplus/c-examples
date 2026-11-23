/*
 * fly_away_adventure.c - a small text adventure at the airport.
 *
 * Flight MC101 leaves in 15 minutes. Find your passport, check in, get
 * through security and board at the right gate before the clock runs out.
 * Every action takes a minute; looking around, checking your bag and
 * asking for help are free.
 *
 * Build with GCC or Clang:
 *     gcc -std=c11 -Wall -Wextra -o fly_away_adventure fly_away_adventure.c
 * Build with Visual Studio (Developer Command Prompt):
 *     cl /W4 fly_away_adventure.c
 * Run:
 *     ./fly_away_adventure
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define LINE_SIZE     64
#define START_MINUTES 15
#define CARRIED       (-1)   /* item location: in the player's bag */

typedef enum { NOWHERE, CURB, CHECK_IN, SECURITY, DEPARTURES, CAFE, GATE_A, GATE_B, ROOM_COUNT } RoomId;
typedef enum { NORTH, SOUTH, EAST, WEST, DIR_COUNT } Direction;
typedef enum { PASSPORT, BOARDING_PASS, WATER, COFFEE, ITEM_COUNT } ItemId;
typedef enum { CMD_NONE, CMD_GO, CMD_TAKE, CMD_DROP, CMD_LOOK, CMD_INVENTORY,
               CMD_BOARD, CMD_HELP, CMD_QUIT, CMD_UNKNOWN } Command;
typedef enum { PLAYING, WON, LOST, GAVE_UP } Status;

/* One place in the world: its name, its description and where each exit leads. */
typedef struct {
    const char *name;
    const char *description;
    int exits[DIR_COUNT];      /* RoomId for each direction; NOWHERE (0) if none */
} Room;

/* Everything that changes while you play. */
typedef struct {
    int    room;                    /* current RoomId */
    int    minutes;                 /* minutes left */
    int    item_at[ITEM_COUNT];     /* RoomId, or CARRIED */
    Status status;
} Game;

/* The map is data, not code: one row per room. [CHECK_IN] = {...} is a
 * designated initializer: it fills that element by name, and inside the
 * exits array [NORTH] = SECURITY does the same. Anything not listed is set
 * to 0, and NOWHERE is the first enum value, 0, so a missing exit needs no
 * extra code. Adding a room means adding a row. */
static const Room rooms[ROOM_COUNT] = {
    [CURB]       = { "Taxi drop-off",
                     "Your taxi is about to pull away. The terminal doors are to the north.",
                     { [NORTH] = CHECK_IN } },
    [CHECK_IN]   = { "Check-in hall",
                     "A check-in agent waves you over. Security is to the north.",
                     { [NORTH] = SECURITY, [SOUTH] = CURB } },
    [SECURITY]   = { "Security",
                     "A guard checks boarding passes. A sign says: no liquids past this point.",
                     { [NORTH] = DEPARTURES, [SOUTH] = CHECK_IN } },
    [DEPARTURES] = { "Departures hall",
                     "A screen flashes: MC101 to Lisbon - GATE CHANGE - now boarding at Gate B7. "
                     "Gate A3 is to the west, Gate B7 to the east and a cafe to the north.",
                     { [NORTH] = CAFE, [EAST] = GATE_B, [WEST] = GATE_A } },
    [CAFE]       = { "Cafe",
                     "It smells of fresh coffee. The queue is long.",
                     { [SOUTH] = DEPARTURES } },
    [GATE_A]     = { "Gate A3",
                     "The sign above the desk says: MC204 to Oslo.",
                     { [EAST] = DEPARTURES } },
    [GATE_B]     = { "Gate B7",
                     "MC101 to Lisbon. The last passengers are boarding.",
                     { [WEST] = DEPARTURES } },
};

static const char *const dir_names[DIR_COUNT]   = { "north", "south", "east", "west" };
static const char *const item_names[ITEM_COUNT] = { "passport", "boarding pass", "water", "coffee" };

static int     read_line(char *buf, size_t size);
static void    describe_room(const Game *g);
static Command parse_command(char *line, char **noun);
static int     direction_from_word(const char *word);
static int     item_from_name(const char *name);
static int     carrying(const Game *g, int item);
static int     do_go(Game *g, int dir);
static int     do_take(Game *g, const char *noun);
static int     do_drop(Game *g, const char *noun);
static int     do_board(Game *g);
static void    show_inventory(const Game *g);
static void    show_help(void);

int main(int argc, char *argv[])
{
    Game game = { CURB, START_MINUTES, { CURB, CHECK_IN, CARRIED, CAFE }, PLAYING };
    char line[LINE_SIZE];
    char *noun = NULL;

    (void)argv;
    if (argc != 1) {
        fprintf(stderr, "Usage: fly_away_adventure\n");
        return 1;
    }

    printf("Fly Away\n========\n");
    printf("Your flight MC101 to Lisbon leaves in %d minutes. Get to the gate and board!\n", START_MINUTES);
    printf("Every action takes a minute. Looking around, checking your bag and help are free.\n");
    printf("Type help for the commands.\n");
    describe_room(&game);

    while (game.status == PLAYING) {
        int cost = 0, dir = -1;

        printf("[%d min] > ", game.minutes);
        fflush(stdout);
        if (!read_line(line, sizeof line)) {
            printf("\n\nGoodbye!\n");          /* input ended */
            return 0;
        }
        switch (parse_command(line, &noun)) {   /* line now holds just the verb */
        case CMD_NONE:      break;
        case CMD_GO:
            dir = direction_from_word(strcmp(line, "go") == 0 ? noun : line);
            if (dir < 0)
                printf("Go where? Try north, south, east or west.\n");
            else
                cost = do_go(&game, dir);
            break;
        case CMD_TAKE:      cost = do_take(&game, noun); break;
        case CMD_DROP:      cost = do_drop(&game, noun); break;
        case CMD_LOOK:      describe_room(&game); break;
        case CMD_INVENTORY: show_inventory(&game); break;
        case CMD_BOARD:     cost = do_board(&game); break;
        case CMD_HELP:      show_help(); break;
        case CMD_QUIT:      game.status = GAVE_UP; break;
        case CMD_UNKNOWN:
            printf("I don't understand \"%s\". Type help for the commands.\n", line);
            break;
        }
        game.minutes -= cost;                   /* the time rule lives here, once */
        if (game.status == PLAYING && game.minutes <= 0)
            game.status = LOST;                 /* boarding on the last minute still wins */
    }

    printf("\n");
    switch (game.status) {
    case WON:
        printf("You board MC101 with %d minute%s to spare. Have a good trip!\n",
               game.minutes, game.minutes == 1 ? "" : "s");
        break;
    case LOST:
        printf("The gate has closed. You missed your flight.\n");
        break;
    default:
        printf("You give up and take a taxi home.\n");
        break;
    }
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

/* Prints a blank line, the room's name and description, its exits in
 * direction order and, if any, the items lying here in item order. */
static void describe_room(const Game *g)
{
    const Room *r = &rooms[g->room];
    const char *sep = "";

    printf("\n== %s ==\n%s\nExits: ", r->name, r->description);
    for (int d = 0; d < DIR_COUNT; d++) {
        if (r->exits[d] != NOWHERE) {
            printf("%s%s", sep, dir_names[d]);
            sep = ", ";
        }
    }
    printf("\n");
    sep = "You see: ";
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (g->item_at[i] == g->room) {
            printf("%s%s", sep, item_names[i]);
            sep = ", ";
        }
    }
    if (sep[0] == ',')                          /* at least one item was listed */
        printf("\n");
}

/* Trims and lowercases line in place, then splits it at the first space:
 * line keeps the verb and *noun points at the rest (or at "").
 * Words are compared with strcmp(): == would compare the addresses of
 * the two strings, not their text, and would almost never be true.
 * Returns the command the verb names; directions give CMD_GO. */
static Command parse_command(char *line, char **noun)
{
    char *start = line, *end = line + strlen(line), *space;

    while (*start == ' ' || *start == '\t')
        start++;
    while (end > start && (end[-1] == ' ' || end[-1] == '\t'))
        end--;
    *end = '\0';
    memmove(line, start, strlen(start) + 1);    /* the trimmed text now starts at line */
    for (char *p = line; *p != '\0'; p++)
        *p = (char)tolower((unsigned char)*p);

    *noun = line + strlen(line);                /* "" unless there is a second word */
    space = strchr(line, ' ');
    if (space != NULL) {
        *space = '\0';
        *noun = space + 1;
        while (**noun == ' ' || **noun == '\t')
            (*noun)++;
    }

    if (line[0] == '\0')                       return CMD_NONE;
    if (direction_from_word(line) >= 0)        return CMD_GO;
    if (strcmp(line, "go") == 0)               return CMD_GO;
    if (strcmp(line, "take") == 0)             return CMD_TAKE;
    if (strcmp(line, "drop") == 0)             return CMD_DROP;
    if (strcmp(line, "look") == 0)             return CMD_LOOK;
    if (strcmp(line, "inventory") == 0 || strcmp(line, "i") == 0)
        return CMD_INVENTORY;
    if (strcmp(line, "board") == 0)            return CMD_BOARD;
    if (strcmp(line, "help") == 0)             return CMD_HELP;
    if (strcmp(line, "quit") == 0)             return CMD_QUIT;
    return CMD_UNKNOWN;
}

/* Returns the Direction for "north" or "n" (and so on), or -1 if word is
 * not a direction. */
static int direction_from_word(const char *word)
{
    for (int d = 0; d < DIR_COUNT; d++)
        if (strcmp(word, dir_names[d]) == 0 || (word[0] == dir_names[d][0] && word[1] == '\0'))
            return d;
    return -1;
}

/* Returns the ItemId for an item's name, or -1 if there is no such item. */
static int item_from_name(const char *name)
{
    for (int i = 0; i < ITEM_COUNT; i++)
        if (strcmp(name, item_names[i]) == 0)
            return i;
    return -1;
}

/* Returns 1 if the player is carrying item, 0 otherwise. */
static int carrying(const Game *g, int item)
{
    return g->item_at[item] == CARRIED;
}

/* Walks one room in direction dir. Security will not let you north without
 * a boarding pass, or with water. Returns the cost: 1 minute, even when
 * the way is blocked. */
static int do_go(Game *g, int dir)
{
    int next = rooms[g->room].exits[dir];

    if (next == NOWHERE) {
        printf("You can't go that way.\n");
        return 1;
    }
    if (g->room == SECURITY && dir == NORTH) {
        if (!carrying(g, BOARDING_PASS)) {
            printf("The guard says: \"Boarding pass, please.\"\n");
            return 1;
        }
        if (carrying(g, WATER)) {
            printf("The guard points at your water: no liquids past this point.\n");
            return 1;
        }
    }
    g->room = next;
    describe_room(g);
    return 1;
}

/* Picks up the item named noun if it is here. Check-in only hands over the
 * boarding pass to someone carrying a passport. Returns the cost: 0 if no
 * item was named, 3 for the coffee queue, otherwise 1. */
static int do_take(Game *g, const char *noun)
{
    int item = item_from_name(noun);

    if (noun[0] == '\0') {
        printf("Take what?\n");
        return 0;
    }
    if (item < 0 || g->item_at[item] != g->room) {
        printf("There is no %s here.\n", noun);
        return 1;
    }
    if (item == BOARDING_PASS && !carrying(g, PASSPORT)) {
        printf("The agent says: \"Passport, please.\"\n");
        return 1;
    }
    g->item_at[item] = CARRIED;
    switch (item) {
    case BOARDING_PASS:
        printf("The agent hands you a boarding pass: MC101, Gate A3.\n");
        return 1;
    case COFFEE:
        printf("The queue is long. Three minutes later, you have a coffee.\n");
        return 3;
    default:
        printf("You take the %s.\n", item_names[item]);
        return 1;
    }
}

/* Puts down the item named noun in the current room. Returns the cost:
 * 0 if no item was named, otherwise 1. */
static int do_drop(Game *g, const char *noun)
{
    int item = item_from_name(noun);

    if (noun[0] == '\0') {
        printf("Drop what?\n");
        return 0;
    }
    if (item < 0 || !carrying(g, item)) {
        printf("You don't have the %s.\n", noun);
        return 1;
    }
    g->item_at[item] = g->room;
    printf("You drop the %s.\n", item_names[item]);
    return 1;
}

/* Boards the plane at Gate B7 with a boarding pass, which wins the game.
 * Returns the cost: 1 minute, even when boarding fails. */
static int do_board(Game *g)
{
    if (g->room == GATE_B && carrying(g, BOARDING_PASS))
        g->status = WON;
    else if (g->room == GATE_B)
        printf("The agent says: \"Boarding pass, please.\"\n");
    else if (g->room == GATE_A)
        printf("This gate is for MC204 to Oslo. Check the departures screen.\n");
    else
        printf("There is no plane here.\n");
    return 1;
}

/* Prints the items being carried, in item order, or that there are none. */
static void show_inventory(const Game *g)
{
    const char *sep = "You are carrying: ";

    for (int i = 0; i < ITEM_COUNT; i++) {
        if (carrying(g, i)) {
            printf("%s%s", sep, item_names[i]);
            sep = ", ";
        }
    }
    if (sep[0] == ',')
        printf(".\n");
    else
        printf("You are carrying nothing.\n");
}

/* Prints the list of commands. */
static void show_help(void)
{
    printf("Commands:\n"
           "  n, s, e, w      walk north, south, east or west (or: go north)\n"
           "  take ITEM       pick something up\n"
           "  drop ITEM       put something down\n"
           "  look            describe this place again (free)\n"
           "  inventory, i    list what you carry (free)\n"
           "  board           board the plane\n"
           "  help            show this list (free)\n"
           "  quit            give up\n");
}
