#ifndef __COMMAND_PRINTF_H
#define __COMMAND_PRINTF_H

#include <xkernel/stdarg.h>
#include <stdint.h>

uint64_t my_vsprintf(char* buf,const char* format,va_list ap);
int myprintf(const char *fmt, ...);
char *ustrcpy(char *dest, const char *src);
char* ustrcat(char* dst_, const char* src_);
void umemset(void* dst_, uint8_t value, uint32_t size);

#endif
