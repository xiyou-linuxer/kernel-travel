#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#include <linux/types.h>
#include <linux/list.h>
#include <asm/thread_info.h>

#define CONFIG_ARCH_TASK_STRUCT_ON_STACK

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
struct thread_stack;

enum task_status {
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_WAITING,
	TASK_HANGING,
	TASK_DIED
};

struct thread_stack {
	/* Main processor registers. */
	unsigned long reg01, reg03, reg22; /* ra sp fp */
	unsigned long reg23, reg24, reg25, reg26; /* s0-s3 */
	unsigned long reg27, reg28, reg29, reg30, reg31; /* s4-s8 */

	/* __schedule() return address / call frame address */
	unsigned long sched_ra;
	unsigned long sched_cfa;

	/* CSR registers */
	unsigned long csr_prmd;
	unsigned long csr_crmd;
	unsigned long csr_euen;
	unsigned long csr_ecfg;
	unsigned long csr_badvaddr; /* Last user fault */

	/* Scratch registers */
	unsigned long scr0;
	unsigned long scr1;
	unsigned long scr2;
	unsigned long scr3;

	/* Eflags register */
	unsigned long eflags;
};

struct task_struct {
	uint64_t *self_kstack;
	thread_func *function;
	void *func_arg;
	struct thread_stack thread;
	pid_t pid;
	enum task_status status;
	char name[TASK_NAME_LEN];
	uint8_t priority;
	uint8_t ticks;
	uint32_t elapsed_ticks;
	struct list_elem general_tag;
	struct list_elem all_list_tag;
	uint64_t pgdir;
	uint32_t stack_magic;
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
