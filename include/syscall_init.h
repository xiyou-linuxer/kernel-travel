#ifndef  _SYSCALL_INIT_H
#define  _SYSCALL_INIT_H
#include <asm/syscall.h>
#include <xkernel/thread.h>

void sys_pstr(char *str);   //权宜之计，暂时写这儿
void syscall_init(void);

#endif
