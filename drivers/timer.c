#include <timer.h>
#include <thread.h>
#include <debug.h>
#include <linux/printk.h>
#include <asm/pt_regs.h>
#include <asm/loongarch.h>
#include <trap/irq.h>
#include <trap/softirq.h>
#include <linux/stdio.h>

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
    printk("%d",ticks);

    /*
    if (cur_thread->ticks == 0) {
        schedule();
    } else {
        cur_thread->ticks--;
    }
    */
}

void schedule()
{
    ASSERT(intr_get_status() == INTR_OFF);

    struct task_struct* cur = running_thread(); 
    if (cur->status == TASK_RUNNING) {
        ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
        list_append(&thread_ready_list, &cur->general_tag);
        cur->ticks = cur->priority;    
        cur->status = TASK_READY;
    } else { 

    }

    /*
    if (list_empty(&thread_ready_list)) {
        thread_unblock(idle_thread);
    }
    */

    ASSERT(!list_empty(&thread_ready_list));
    thread_tag = NULL;	  // thread_tag清空
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;


    //switch_to(cur, next);
}

void timer_init() {
    printk("timer_init start\n");

    register_handler(EXCCODE_TIMER, intr_timer_handler);

    printk("timer_init done\n");
}

