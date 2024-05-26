#ifndef __FORK_H
#define __FORK_H
#include <xkernel/thread.h>

pid_t sys_fork(int (*fn)(void *arg),void *stack,unsigned long flags,void *arg);

#endif
