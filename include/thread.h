#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include <list.h>
#include <stdint.h>

#define TASK_NAME_LEN 16
#define MAX_FILES_OPEN_PER_PROC 8

#define STACK_MAGIC_NUM 0x27839128

typedef void thread_func(void*);
typedef int16_t pid_t;

enum task_status {
   TASK_RUNNING,
   TASK_READY,
   TASK_BLOCKED,
   TASK_WAITING,
   TASK_HANGING,
   TASK_DIED
};

extern struct list thread_ready_list;
extern struct list thread_all_list;
extern struct list_elem* thread_tag;


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
    uint32_t stack_magic;
};

void init_thread(struct task_struct *pthread, char *name, int prio);
struct task_struct* running_thread(void);
void thread_init(void);
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg);
struct task_struct *thread_start(char *name, int prio, thread_func function, void *func_arg);
void schedule(void);
void thread_block(enum task_status stat);
void thread_unblock(struct task_struct* thread);
void release_pid(pid_t pid);

#endif

