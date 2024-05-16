#ifndef __KERNEL_SOFTINTERRUPT_H
#define __KERNEL_SOFTINTERRUPT_H
#include <stdint.h>
#include <xkernel/init.h>

extern uint32_t softirq_active;
extern uint32_t irq_count;
extern uint32_t bh_count;

enum
{
    TIMER_SOFTIRQ,
};

struct softirq_action
{
    void (*action) (void*);
    void *data;
};

void __init softirq_init(void);
void do_softirq(void);
void open_softirq(uint32_t nr,void (*action)(void*),void* data);
void raise_softirq(uint32_t nr);

#endif
