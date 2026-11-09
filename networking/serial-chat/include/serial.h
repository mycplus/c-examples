#ifndef SERIAL_H
#define SERIAL_H

#include <stdbool.h>
#include <stddef.h>

/* One serial port, opened as 8 data bits, no parity, 1 stop bit, no flow control. */
typedef struct serial_port serial_port;

/* device: "/dev/ttyUSB0" on Linux, "COM3" on Windows. Returns NULL and fills err on failure. */
serial_port *serial_open(const char *device, long baud, char *err, size_t err_len);

/* Writes every byte or returns false. */
bool serial_write_all(serial_port *port, const void *data, size_t len);

/* Waits up to timeout_ms for data. Returns bytes read, 0 on timeout, -1 on error. */
long serial_read(serial_port *port, void *buf, size_t len, int timeout_ms);

void serial_close(serial_port *port);

#endif
