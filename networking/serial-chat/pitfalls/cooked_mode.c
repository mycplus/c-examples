/* What a terminal device in its default "cooked" mode does to binary data,
   compared with raw mode. Uses a pseudo-terminal, so no cable is needed.
   Linux and macOS only. */
#define _DEFAULT_SOURCE
#include <poll.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <util.h>
#else
#include <pty.h>
#endif

static void show(const char *label, int fd)
{
    unsigned char buf[64];
    struct pollfd pfd = {.fd = fd, .events = POLLIN};
    printf("%-10s", label);
    while (poll(&pfd, 1, 200) > 0) {
        ssize_t n = read(fd, buf, sizeof buf);
        if (n <= 0)
            break;
        printf(" [");
        for (ssize_t i = 0; i < n; i++)
            printf("%s%02X", i ? " " : "", buf[i]);
        printf("]");
    }
    printf("\n");
}

static void run(int raw)
{
    int master, slave;
    if (openpty(&master, &slave, NULL, NULL, NULL) != 0)
        return;
    if (raw) {
        struct termios t;
        tcgetattr(slave, &t);
        cfmakeraw(&t);
        tcsetattr(slave, TCSANOW, &t);
    }
    /* 'A', CR, 'B', XOFF, XON, 'C', LF */
    const unsigned char sent[] = {0x41, 0x0D, 0x42, 0x13, 0x11, 0x43, 0x0A};
    if (write(master, sent, sizeof sent) == (ssize_t)sizeof sent)
        show(raw ? "raw:" : "cooked:", slave);
    close(slave);
    close(master);
}

int main(void)
{
    printf("sent:      [41 0D 42 13 11 43 0A]\n");
    run(0);
    run(1);
    return 0;
}
