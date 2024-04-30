#include <asm/timer.h>
#include <linux/thread.h>
#include <debug.h>
#include <linux/printk.h>
#include <asm/pt_regs.h>
#include <asm/loongarch.h>
#include <trap/irq.h>
#include <trap/softirq.h>
#include <linux/stdio.h>
#include <linux/switch.h>

#define IRQ0_FREQUENCY     100
#define INPUT_FREQUENCY    1193180
#define COUNTER0_VALUE     INPUT_FREQUENCY / IRQ0_FREQUENCY
#define CONTRER0_PORT      0x40
#define COUNTER0_NO        0
#define COUNTER_MODE       2
#define READ_WRITE_LATCH   3
#define PIT_CONTROL_PORT   0x43

#define mil_seconds_per_intr (1000 / IRQ0_FREQUENCY)

unsigned long ticks;
unsigned long timer_ticks;

void intr_timer_handler(struct pt_regs *regs)
{
    struct task_struct* cur_thread = running_thread();

    ASSERT(cur_thread->stack_magic == STACK_MAGIC_NUM);

    cur_thread->elapsed_ticks++;
    ticks++;

    write_csr_ticlr(read_csr_ticlr() | (0x1 << 0));
    //printk("%d",cur_thread->ticks--);
    printk("%d",cur_thread->ticks);

    if (cur_thread->ticks == 0) {
        schedule();
    } else {
        cur_thread->ticks--;
    }
}


void timer_init() {
    printk("timer_init start\n");

    register_handler(EXCCODE_TIMER, intr_timer_handler);

    printk("timer_init done\n");
}

