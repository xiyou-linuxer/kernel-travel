#include <xkernel/stdio.h>
#include <xkernel/printk.h>
#include <xkernel/efi.h>
#include <xkernel/types.h>
#include <xkernel/kernel.h>
#include <asm/syscall.h>

#include <asm/stdio.h>

int printk(const char *fmt, ...)
{
    // print_str("111");
    char printf_buf[1024];
    va_list args;
	int printed;
	int loglevel = printk_get_level(fmt);

	switch (loglevel) {
	case '0' ... '9':
		loglevel -= '0';
		break;
	default:
		loglevel = -1;
		break;
	}

	fmt = printk_skip_level(fmt);

	va_start(args, fmt);


    printed = vsnprintf(printf_buf, sizeof(printf_buf), fmt, args);
	va_end(args);

	print_str(printf_buf);
#ifndef CONFIG_RISCV
	if (printed >= sizeof(printf_buf)) {
		print_str("[Message truncated]\n");
		return -1;
	}

	return printed;
	#endif
	return 0;
}
