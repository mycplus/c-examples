/* Serial port access for Linux and macOS, using termios. */
#define _DEFAULT_SOURCE
#include "serial.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

struct serial_port {
    int fd;
};

static speed_t baud_constant(long baud)
{
    switch (baud) {
    case 9600:   return B9600;
    case 19200:  return B19200;
    case 38400:  return B38400;
    case 57600:  return B57600;
    case 115200: return B115200;
    default:     return 0;
    }
}

serial_port *serial_open(const char *device, long baud, char *err, size_t err_len)
{
    speed_t speed = baud_constant(baud);
    if (speed == 0) {
        snprintf(err, err_len, "unsupported baud rate %ld", baud);
        return NULL;
    }

    /* O_NOCTTY: this port must not become our controlling terminal. */
    int fd = open(device, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        snprintf(err, err_len, "%s: %s", device, strerror(errno));
        return NULL;
    }

    struct termios tio;
    if (tcgetattr(fd, &tio) != 0) {
        snprintf(err, err_len, "%s: not a terminal device: %s", device, strerror(errno));
        close(fd);
        return NULL;
    }

    /* Raw mode: no line editing, no echo, no CR/LF translation, no signals. */
    tio.c_iflag &= ~(tcflag_t)(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF);
    tio.c_oflag &= ~(tcflag_t)OPOST;
    tio.c_lflag &= ~(tcflag_t)(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

    /* 8N1, ignore modem-control lines, enable the receiver. */
    tio.c_cflag &= ~(tcflag_t)(CSIZE | PARENB | CSTOPB);
    tio.c_cflag |= CS8 | CLOCAL | CREAD;
#ifdef CRTSCTS
    tio.c_cflag &= ~(tcflag_t)CRTSCTS;   /* a three-wire cable has no RTS/CTS */
#endif

    /* read() returns whatever is available; poll() does the waiting. */
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;

    if (cfsetispeed(&tio, speed) != 0 || cfsetospeed(&tio, speed) != 0 ||
        tcsetattr(fd, TCSANOW, &tio) != 0) {
        snprintf(err, err_len, "%s: cannot configure: %s", device, strerror(errno));
        close(fd);
        return NULL;
    }
    tcflush(fd, TCIOFLUSH);           /* drop anything received before we were ready */

    serial_port *port = malloc(sizeof *port);
    if (port == NULL) {
        snprintf(err, err_len, "out of memory");
        close(fd);
        return NULL;
    }
    port->fd = fd;
    return port;
}

bool serial_write_all(serial_port *port, const void *data, size_t len)
{
    const unsigned char *p = data;
    while (len > 0) {
        ssize_t n = write(port->fd, p, len);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return false;
        }
        p += n;
        len -= (size_t)n;
    }
    return true;
}

long serial_read(serial_port *port, void *buf, size_t len, int timeout_ms)
{
    struct pollfd pfd = {.fd = port->fd, .events = POLLIN};
    int ready = poll(&pfd, 1, timeout_ms);
    if (ready < 0)
        return errno == EINTR ? 0 : -1;
    if (ready == 0)
        return 0;
    ssize_t n = read(port->fd, buf, len);
    if (n < 0)
        return errno == EINTR || errno == EAGAIN ? 0 : -1;
    if (n == 0 && (pfd.revents & POLLHUP))
        return -1;                     /* the other end went away */
    return (long)n;
}

void serial_close(serial_port *port)
{
    if (port != NULL) {
        close(port->fd);
        free(port);
    }
}
