#include <asm/stdio.h>
#include <linux/ns16550a.h>

void print_str(char *str)
{
	serial_ns16550a_puts(str);
}
