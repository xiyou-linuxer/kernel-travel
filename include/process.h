#ifndef __PROCESS_H
#define __PROCESS_H
#include <linux/thread.h>
#include <allocator.h>

#define USER_STACK_START  (USER_STACK - 0x1000)
#define USER_VADDR_START 0x8048000

extern void proc_1(void *);

void start_process(void* filename);
void process_execute(void* filename, char* name);
void page_dir_activate(struct task_struct* pcb);


#endif
