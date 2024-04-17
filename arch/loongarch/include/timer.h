#ifndef __TIMER_H
#define __TIMER_H
#include <stdint.h>
#include <list.h>
#include <asm/pt_regs.h>

extern unsigned long ticks;
extern void intr_timer_handler(struct pt_regs *regs);

void timer_init(void);
void schedule(void);

#endif
