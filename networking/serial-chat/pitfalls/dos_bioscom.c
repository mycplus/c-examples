/* DOS-era serial code, as written for Turbo C. It does not build with a modern
   compiler: the headers and the direct port access exist only under DOS. */
#include <bios.h>
#include <dos.h>

int main(void)
{
    bioscom(0, 0xE3, 0);          /* COM1: 9600 baud, 8 data bits, no parity, 1 stop */
    outportb(0x3F8, 'A');         /* write straight to the UART's I/O port */
    return 0;
}
