/* Serial port access for Windows, using the Win32 communications API. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "serial.h"

#include <stdio.h>
#include <stdlib.h>

struct serial_port {
    HANDLE handle;
};

static void win_error(char *err, size_t err_len, const char *what)
{
    snprintf(err, err_len, "%s failed (Windows error %lu)", what, (unsigned long)GetLastError());
}

serial_port *serial_open(const char *device, long baud, char *err, size_t err_len)
{
    /* "\\.\COM10": names above COM9 only open with the device-namespace prefix. */
    char path[64];
    snprintf(path, sizeof path, "\\\\.\\%s", device);

    HANDLE h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        win_error(err, err_len, "CreateFile");
        return NULL;
    }

    DCB dcb;
    SecureZeroMemory(&dcb, sizeof dcb);
    dcb.DCBlength = sizeof dcb;
    if (!GetCommState(h, &dcb)) {
        win_error(err, err_len, "GetCommState");
        CloseHandle(h);
        return NULL;
    }
    dcb.BaudRate = (DWORD)baud;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;              /* a three-wire cable has no handshake lines */
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fOutX = FALSE;                     /* no XON/XOFF: binary data may contain those bytes */
    dcb.fInX = FALSE;
    dcb.fNull = FALSE;
    dcb.fAbortOnError = FALSE;
    if (!SetCommState(h, &dcb)) {
        win_error(err, err_len, "SetCommState");
        CloseHandle(h);
        return NULL;
    }
    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);

    serial_port *port = malloc(sizeof *port);
    if (port == NULL) {
        snprintf(err, err_len, "out of memory");
        CloseHandle(h);
        return NULL;
    }
    port->handle = h;
    return port;
}

bool serial_write_all(serial_port *port, const void *data, size_t len)
{
    const unsigned char *p = data;
    while (len > 0) {
        DWORD chunk = len > 65536 ? 65536 : (DWORD)len;
        DWORD written = 0;
        if (!WriteFile(port->handle, p, chunk, &written, NULL) || written == 0)
            return false;
        p += written;
        len -= written;
    }
    return true;
}

long serial_read(serial_port *port, void *buf, size_t len, int timeout_ms)
{
    /* Return as soon as any byte arrives, or after timeout_ms with nothing. */
    COMMTIMEOUTS t = {0};
    t.ReadIntervalTimeout = MAXDWORD;
    t.ReadTotalTimeoutMultiplier = MAXDWORD;
    t.ReadTotalTimeoutConstant = timeout_ms > 0 ? (DWORD)timeout_ms : 1;
    if (!SetCommTimeouts(port->handle, &t))
        return -1;

    DWORD got = 0;
    DWORD want = len > 65536 ? 65536 : (DWORD)len;
    if (!ReadFile(port->handle, buf, want, &got, NULL))
        return -1;
    return (long)got;
}

void serial_close(serial_port *port)
{
    if (port != NULL) {
        CloseHandle(port->handle);
        free(port);
    }
}
