#ifndef __PROCESS_H
#define __PROCESS_H
#include <linux/thread.h>

void start_process(void* filename);
void process_execute(void* filename, char* name);
void page_dir_activate(struct task_struct* pcb);


#endif
