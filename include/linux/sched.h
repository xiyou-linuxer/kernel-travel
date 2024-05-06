#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/memory.h>
#include <asm/thread_info.h>
#include <linux/memory.h>

#define TASK_NAME_LEN 16
#define MAX_FILES_OPEN_PER_PROC 8

#define STACK_MAGIC_NUM 0x27839128

// struct task_struct {
// #ifdef CONFIG_THREAD_INFO_IN_TASK
// #endif
// 	unsigned int __state;
// 	void *stack;
// };

typedef int16_t pid_t;

typedef void thread_func(void *);

enum task_status {
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_WAITING,
	TASK_HANGING,
	TASK_DIED
};

struct task_struct {
	uint64_t *self_kstack;
	thread_func *function;
	void *func_arg;
	struct thread_struct thread;
	pid_t pid;
	enum task_status status;
	char name[TASK_NAME_LEN];
	uint8_t priority;
	uint8_t ticks;
	uint32_t elapsed_ticks;
	struct virt_addr usrprog_vaddr;
	struct list_elem general_tag;
	struct list_elem all_list_tag;
	uint64_t pgdir;
	u32 asid;
	struct mm_struct *mm;
	uint32_t stack_magic;
};

union thread_union {
	struct thread_info thread_info;
	unsigned long stack[THREAD_SIZE / sizeof(long)];
};

extern struct thread_info init_thread_info;

extern unsigned long init_stack[THREAD_SIZE / sizeof(unsigned long)];

#endif /* _LINUX_SCHED_H */
