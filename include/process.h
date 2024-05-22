#ifndef __PROCESS_H
#define __PROCESS_H
#include <xkernel/thread.h>
#include <allocator.h>

#define USER_STACK_START  (USER_STACK - 0x1000)
#define USER_VADDR_START 0

extern void proc_1(void *);

void pro_seek(void **program,int offset);
void pro_read(void *program,void *buf,uint64_t count);
void start_process(void* filename);
void process_execute(void* filename, char* name,int pri);
void page_dir_activate(struct task_struct* pcb);


#endif
