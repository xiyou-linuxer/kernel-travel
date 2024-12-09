#ifndef _LINUX_KERNEL_H
#define _LINUX_KERNEL_H

#include <xkernel/types.h>
#include <xkernel/stdarg.h>
#include <xkernel/compiler.h>

#define _RET_IP_		((unsigned long)__builtin_return_address(0))

int __must_check kstrtouint(const char *s, unsigned int base, unsigned int *res);
int __must_check kstrtoint(const char *s, unsigned int base, int *res);

int __must_check kstrtou16(const char *s, unsigned int base, u16 *res);
int __must_check kstrtos16(const char *s, unsigned int base, s16 *res);
int __must_check kstrtou8(const char *s, unsigned int base, u8 *res);
int __must_check kstrtos8(const char *s, unsigned int base, s8 *res);
int __must_check kstrtobool(const char *s, bool *res);

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

#endif /* _LINUX_KERNEL_H */
