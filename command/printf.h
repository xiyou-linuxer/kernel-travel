#ifndef __COMMAND_PRINTF_H
#define __COMMAND_PRINTF_H

#include <linux/stdarg.h>
#include <stdint.h>

uint64_t my_vsprintf(char* buf,const char* format,va_list ap);
int myprintf(const char *fmt, ...);

#endif
