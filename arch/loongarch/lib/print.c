#include <asm/stdio.h>
#include <xkernel/ns16550a.h>

void print_str(char *str)
{
	serial_ns16550a_puts(str);
}
