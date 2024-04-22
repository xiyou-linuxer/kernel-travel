#ifndef _LINUX_KERNEL_H
#define _LINUX_KERNEL_H

#include <linux/types.h>
#include <linux/stdarg.h>

#define _RET_IP_		((unsigned long)__builtin_return_address(0))


int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

#endif /* _LINUX_KERNEL_H */
