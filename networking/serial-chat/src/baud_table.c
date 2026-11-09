/* What a baud rate means in bytes. 8N1 sends 10 bits per byte: start, 8 data, stop. */
#include <stdio.h>

int main(void)
{
    const long rates[] = {9600, 19200, 38400, 57600, 115200};
    const double mib = 1024.0 * 1024.0;
    const double payload = 64.0, overhead = 4.0;     /* two END bytes and a 2-byte CRC */

    printf("%8s %12s %16s %22s\n", "baud", "bytes/s", "1 MiB raw", "64-byte messages/s");
    for (size_t i = 0; i < sizeof rates / sizeof rates[0]; i++) {
        double bytes_per_s = rates[i] / 10.0;
        printf("%8ld %12.0f %14.1f s %22.1f\n", rates[i], bytes_per_s, mib / bytes_per_s,
               bytes_per_s / (payload + overhead));
    }
    return 0;
}
