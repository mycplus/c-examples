/* term.c - keyboard, output and timing for Windows consoles and POSIX
 * terminals (Linux, macOS). Everything platform-specific is in this file. */
#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif
#include "term.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_str(const char *s)
{
    term_write(s, strlen(s));
}

#if defined(_WIN32)
#include <conio.h>
#include <windows.h>

static HANDLE out_handle;
static DWORD  saved_mode;
static bool   active;
static volatile LONG interrupted;

static BOOL WINAPI on_ctrl(DWORD type)
{
    (void)type;
    InterlockedExchange(&interrupted, 1);   /* the main loop sees KEY_QUIT next */
    return TRUE;
}

bool term_init(void)
{
    out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out_handle == INVALID_HANDLE_VALUE || !GetConsoleMode(out_handle, &saved_mode))
        return false;
    /* Windows 10 and later interpret ANSI escape sequences once asked to. */
    if (!SetConsoleMode(out_handle, saved_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        return false;
    SetConsoleCtrlHandler(on_ctrl, TRUE);             /* Ctrl+C restores too */
    active = true;
    write_str("\x1b[?25l");                          /* hide the cursor */
    return true;
}

void term_restore(void)
{
    if (!active)
        return;
    write_str("\x1b[?25h\x1b[0m\r\n");
    SetConsoleMode(out_handle, saved_mode);
    active = false;
}

Key term_key(void)
{
    if (InterlockedExchange(&interrupted, 0))     /* report Ctrl+C once */
        return KEY_QUIT;
    while (_kbhit()) {
        int c = _getch();
        if (c == 0 || c == 224) {                     /* extended key prefix */
            int code = _getch();
            if (code == 75) return KEY_LEFT;
            if (code == 77) return KEY_RIGHT;
            continue;
        }
        if (c == 'a' || c == 'A') return KEY_LEFT;
        if (c == 'd' || c == 'D') return KEY_RIGHT;
        if (c == 'p' || c == 'P' || c == ' ') return KEY_PAUSE;
        if (c == 'q' || c == 'Q' || c == 27) return KEY_QUIT;
    }
    return KEY_NONE;
}

uint64_t term_now_ms(void)
{
    return GetTickCount64();
}

void term_sleep_ms(uint32_t ms)
{
    Sleep(ms);
}

#else /* POSIX */
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static struct termios saved;
static bool active;
static volatile sig_atomic_t interrupted;

static void on_signal(int sig)
{
    (void)sig;
    interrupted = 1;                  /* the main loop sees KEY_QUIT next */
}

bool term_init(void)
{
    if (!isatty(STDIN_FILENO) || tcgetattr(STDIN_FILENO, &saved) != 0)
        return false;
    struct termios raw = saved;
    raw.c_lflag &= ~(tcflag_t)(ICANON | ECHO);    /* no line buffering, no echo */
    raw.c_cc[VMIN] = 0;                           /* read() returns at once     */
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0)
        return false;
    active = true;

    struct sigaction sa = {0};
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    write_str("\x1b[?25l");
    return true;
}

void term_restore(void)
{
    if (!active)
        return;
    write_str("\x1b[?25h\x1b[0m\r\n");
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved);
    active = false;
}

static int read_byte(void)
{
    unsigned char c;
    return read(STDIN_FILENO, &c, 1) == 1 ? c : -1;
}

Key term_key(void)
{
    if (interrupted) {                            /* report the signal once */
        interrupted = 0;
        return KEY_QUIT;
    }
    int c;
    while ((c = read_byte()) != -1) {
        if (c == 27) {                                /* ESC [ C / ESC [ D */
            int c2 = read_byte();
            if (c2 == -1)
                return KEY_QUIT;                      /* a lone Esc */
            if (c2 == '[') {
                int c3 = read_byte();
                if (c3 == 'D') return KEY_LEFT;
                if (c3 == 'C') return KEY_RIGHT;
            }
            continue;
        }
        if (c == 'a' || c == 'A') return KEY_LEFT;
        if (c == 'd' || c == 'D') return KEY_RIGHT;
        if (c == 'p' || c == 'P' || c == ' ') return KEY_PAUSE;
        if (c == 'q' || c == 'Q') return KEY_QUIT;
    }
    return KEY_NONE;
}

uint64_t term_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

void term_sleep_ms(uint32_t ms)
{
    struct timespec ts = { (time_t)(ms / 1000u), (long)(ms % 1000u) * 1000000L };
    while (nanosleep(&ts, &ts) != 0 && !interrupted)
        ;                             /* resume after an unrelated signal */
}
#endif

void term_write(const char *s, size_t len)
{
    fwrite(s, 1, len, stdout);
    fflush(stdout);
}
