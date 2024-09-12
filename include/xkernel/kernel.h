#ifndef _LINUX_KERNEL_H
#define _LINUX_KERNEL_H

#include <xkernel/types.h>
#include <xkernel/stdarg.h>

#define _RET_IP_		((unsigned long)__builtin_return_address(0))

#define ALIGN_MASK(x,a)		__ALIGN_MASK(x,(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))
#define ULONG_MAX	(~0UL)

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

#endif /* _LINUX_KERNEL_H */
