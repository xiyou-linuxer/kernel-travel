#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#include <asm/thread_info.h>

#define CONFIG_ARCH_TASK_STRUCT_ON_STACK

struct task_struct {
#ifdef CONFIG_THREAD_INFO_IN_TASK
#endif
	unsigned int __state;
	void *stack;
};

union thread_union {
#ifndef CONFIG_ARCH_TASK_STRUCT_ON_STACK
	struct task_struct task;
#endif
#ifndef CONFIG_THREAD_INFO_IN_TASK
	struct thread_info thread_info;
#endif
	unsigned long stack[THREAD_SIZE / sizeof(long)];
};

#ifndef CONFIG_THREAD_INFO_IN_TASK
extern struct thread_info init_thread_info;
#endif

extern unsigned long init_stack[THREAD_SIZE / sizeof(unsigned long)];

#endif /* _LINUX_SCHED_H */
