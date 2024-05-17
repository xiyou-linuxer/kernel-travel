#ifndef __SWICH_H
#define __SWICH_H
#include <xkernel/thread.h>

void switch_to(struct task_struct* cur,struct task_struct* next);
void prepare_switch(struct task_struct* pcb,uint64_t addr,uint64_t ssp);

#endif
