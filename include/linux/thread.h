#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H

#include <linux/sched.h>
#include <linux/list.h>
#include <asm/timer.h>
#include <stdint.h>

extern struct list thread_ready_list;
extern struct list thread_all_list;
extern struct list_elem* thread_tag;

void init_thread(struct task_struct *pthread, char *name, int prio);
struct task_struct *running_thread(void);
void thread_init(void);
void thread_create(struct task_struct *pthread, thread_func function, void *func_arg);
struct task_struct *thread_start(char *name, int prio, thread_func function, void *func_arg);
void schedule(void);
void thread_block(enum task_status stat);
void thread_unblock(struct task_struct *thread);
void thread_yield(void);
pid_t fork_pid(void);
void release_pid(pid_t pid);
int sys_sleep(struct timespec *req,struct timespec *rem);

#endif
