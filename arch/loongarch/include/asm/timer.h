#ifndef __TIMER_H
#define __TIMER_H
#include <stdint.h>
#include <linux/list.h>
#include <asm/pt_regs.h>

extern unsigned long ticks;
extern void intr_timer_handler(struct pt_regs *regs);

#define LVL_DEPTH       7
#define LVL_BITS        6
#define LVL_SIZE         (1UL << LVL_BITS)
#define LVL_MASK         (LVL_SIZE-1)
#define LVL_OFF(n)       ((n) * LVL_SIZE)

#define LVL_SHIFT(n)     ((n) * LVL_BITS)
#define LVL_START(n)     (1UL << LVL_SHIFT(n))

struct timer_vec {
    unsigned int index;
    struct list vec[LVL_SIZE];
};

struct timer_list {
    struct list_elem elm;
    unsigned long expires;
    void (*func)(unsigned long data);
    unsigned long data;
};

void timer_init(void);
void add_timer(struct timer_list *timer);
int mod_timer(struct timer_list *timer,unsigned long expire);
int del_timer(struct timer_list *timer);

#endif
