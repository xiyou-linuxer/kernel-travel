#ifndef __WAIT_H
#define __WAIT_H
#include <xkernel/thread.h>

void sys_exit(int status);
pid_t sys_wait(pid_t pid,int* status,int options);



#endif
