#ifndef __SWICH_H
#define __SWICH_H
#include <linux/thread.h>

void switch_to(struct task_struct* cur,struct task_struct* next);

#endif