#ifndef __DEVICE_CONSOLE_H
#define __DEVICE_CONSOLE_H

#include <xkernel/types.h>
int console_check_read(void);
void console_init(void);
void console_acquire(void);
void console_release(void);
void console_put_str(char* str);
void console_put_char(uint8_t char_asci);
void console_get_str(char* str);
void sys_putchar(uint8_t c);
#endif