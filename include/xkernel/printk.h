#ifndef _LINUX_PRINTK_H
#define _LINUX_PRINTK_H

#include <xkernel/kern_level.h>
#include <xkernel/stdio.h>

#define PRINTK_MAX_SINGLE_HEADER_LEN 2

static inline int printk_get_level(const char *buffer)
{
	if (buffer[0] == KERN_SOH_ASCII && buffer[1]) {
		switch (buffer[1]) {
		case '0' ... '7':
		case 'c':	/* KERN_CONT */
			return buffer[1];
		}
	}
	return 0;
}

static inline const char *printk_skip_level(const char *buffer)
{
	if (printk_get_level(buffer))
		return buffer + 2;

	return buffer;
}

static inline const char *printk_skip_headers(const char *buffer)
{
	while (printk_get_level(buffer))
		buffer = printk_skip_level(buffer);

	return buffer;
}

#define CONSOLE_EXT_LOG_MAX	8192

/* printk's without a loglevel use this.. */
#define MESSAGE_LOGLEVEL_DEFAULT CONFIG_MESSAGE_LOGLEVEL_DEFAULT

/* We show everything that is MORE important than this.. */
#define CONSOLE_LOGLEVEL_SILENT  0 /* Mum's the word */
#define CONSOLE_LOGLEVEL_MIN	 1 /* Minimum loglevel we let people use */
#define CONSOLE_LOGLEVEL_DEBUG	10 /* issue debug messages */
#define CONSOLE_LOGLEVEL_MOTORMOUTH 15	/* You can't shut this one up */

#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#define pr_emerg(fmt, ...) \
	printk(KERN_EMERG "[Emerg/%s]" pr_fmt(fmt), __func__, ##__VA_ARGS__)

#define pr_alert(fmt, ...) \
	printk(KERN_ALERT "[Alert/%s]" pr_fmt(fmt), __func__, ##__VA_ARGS__)

#define pr_crit(fmt, ...) \
	printk(KERN_CRIT "[Crit/%s]" pr_fmt(fmt), __func__, ##__VA_ARGS__)

#define pr_err(fmt, ...) \
	printk(KERN_ERR "[Err/%s]" pr_fmt(fmt), __func__, ##__VA_ARGS__)

#define pr_warn(fmt, ...) \
	printk(KERN_WARNING "[Warning/%s]" pr_fmt(fmt), __func__, ##__VA_ARGS__)

#define pr_notice(fmt, ...) \
	printk(KERN_NOTICE "[Notice/%s]" pr_fmt(fmt), __func__, ##__VA_ARGS__)

#define pr_info(fmt, ...) \
	printk(KERN_INFO "[Info/%s] " pr_fmt(fmt), __func__, ##__VA_ARGS__)

#define pr_debug(fmt, ...) \
	printk(KERN_DEBUG "[Debug/%s] " pr_fmt(fmt), __func__, ##__VA_ARGS__)

void efi_puts(const char *str);

#endif /* _LINUX_PRINTK_H */
