#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H

#include <xkernel/sched.h>
#include <xkernel/list.h>
#include <asm/timer.h>

extern struct list thread_ready_list;
extern struct list thread_all_list;
extern struct list_elem* thread_tag;

pid_t sys_getpid(void);
pid_t sys_getppid(void);
pid_t sys_set_tid_address(int* tidptr);
uid_t sys_getuid(void);
u32 sys_getgid(void);
struct task_struct* pid2thread(int64_t pid);
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
void thread_exit(struct task_struct* exit);

#endif
