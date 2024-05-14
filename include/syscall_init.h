#ifndef  _SYSCALL_INIT_H
#define  _SYSCALL_INIT_H
#include <asm/syscall.h>
#include <linux/thread.h>

pid_t sys_getpid(void);
int64_t sys_write(int fd,const void* buf,size_t count);
void sys_pstr(char *str);   //权宜之计，暂时写这儿
void syscall_init(void);

#endif
