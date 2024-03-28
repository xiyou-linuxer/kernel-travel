#ifndef _LINUX_KERNEL_H
#define _LINUX_KERNEL_H

#include <linux/types.h>
#include <linux/stdarg.h>

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

#endif /* _LINUX_KERNEL_H */
