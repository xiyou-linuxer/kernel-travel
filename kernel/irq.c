#include <asm/loongarch.h>
#include <asm/timer.h>
#include <xkernel/debug.h>
#include <xkernel/printk.h>
#include <xkernel/stdio.h>
#include <xkernel/types.h>
#include <trap/irq.h>
#include <trap/softirq.h>
#include <asm/timer.h>
#include <xkernel/thread.h>
#include <xkernel/memory.h>
#include <xkernel/compiler.h>
#include <xkernel/compiler_types.h>
#include "debug.h"

#define INTR_NUM 256

uint32_t irq_count = 0;
uint32_t bh_count = 0;
extern bool switching;

intr_handler intr_table[INTR_NUM];
char* intr_name[INTR_NUM];

void irq_enter(void) {
	struct task_struct* cur = running_thread();
	utimes_end(cur);
	stimes_begin(cur);

	switching = 0;
	++irq_count;
}

void irq_exit(void) {
	struct task_struct* cur = running_thread();
	stimes_end(cur);

	if (switching) {
		switching = 0;
		return ;
	}
	--irq_count;
}

static void general_intr_handler(struct pt_regs* regs) {
    unsigned long hwirq = *(unsigned long*)regs;
    unsigned long virq = EXCCODE_INT_START + hwirq;
    printk("!!!!!!!      exception message begin  !!!!!!!!\n");
    printk("intr_table[%x]: %s happened", virq, intr_name[virq]);
    printk("\n!!!!!!!      exception message end    !!!!!!!!\n");
    while (1)
        ;
}

static void exception_init(void) {
    int i;
    for (i = 0; i < INTR_NUM; i++) {
        intr_name[i] = "unknown";
        intr_table[i] = general_intr_handler;
    }
}

enum intr_status intr_get_status(void) {
    uint64_t crmd;

    crmd = read_csr_crmd();

    return (crmd & CSR_CRMD_IE) ? INTR_ON : INTR_OFF;
}

static inline void arch_local_irq_enable(void) {
    uint32_t val = CSR_CRMD_IE;
    __asm__ __volatile__(
        "csrxchg %[val], %[mask], %[reg]\n\t"
        : [val] "+r"(val)
        : [mask] "r"(CSR_CRMD_IE), [reg] "i"(LOONGARCH_CSR_CRMD)
        : "memory");
}

static inline void arch_local_irq_disable(void) {
    uint32_t val = 0;
    __asm__ __volatile__(
        "csrxchg %[val], %[mask], %[reg]\n\t"
        : [val] "+r"(val)
        : [mask] "r"(CSR_CRMD_IE), [reg] "i"(LOONGARCH_CSR_CRMD)
        : "memory");
}

/* 开中断并返回开中断前的状态*/
enum intr_status intr_enable(void) {
    enum intr_status old_status;

    if (intr_get_status() == INTR_ON) {
        old_status = INTR_ON;
    } else {
        old_status = INTR_OFF;
        arch_local_irq_enable();
    }

    return old_status;
}

/* 关中断并返回关中断前的状态 */
enum intr_status intr_disable(void) {
    enum intr_status old_status;

    if (intr_get_status() == INTR_ON) {
        old_status = INTR_ON;
        arch_local_irq_disable();
    } else {
        old_status = INTR_OFF;
    }

    return old_status;
}

/* 将中断状态设置为status */
enum intr_status intr_set_status(enum intr_status status) {
    return status & INTR_ON ? intr_enable() : intr_disable();
}

/* 在中断处理程序数组第vector_no个元素中注册安装中断处理程序function */
void register_handler(uint8_t vector_no, intr_handler function) {
    intr_table[vector_no] = function;
}

extern void do_irq(struct pt_regs* regs, uint64_t virq);
void do_irq(struct pt_regs* regs, uint64_t virq) {
	irq_enter();
	intr_table[virq](regs);
	irq_exit();
	if (softirq_active)
		do_softirq();
	utimes_begin(running_thread());
}

bool in_interrupt(void) {
    return (bh_count + irq_count);
}

/*设置中断路由
 *cpu：cpu向量号，0/1
 *IPx：中断引脚号，可填入0～3
 *source_num：中断源号0～63
 */
void irq_routing_set(uint8_t cpu, uint8_t IPx, uint8_t source_num) {
    int intenset, ioentry;
    if (source_num < 32) {
        intenset = INTENSET_0;
        ioentry = IO_ENTRY_0;
    } else {
        intenset = INTENSET_1;
        ioentry = IO_ENTRY_1;
    }
    *(unsigned int*)(CSR_DMW0_BASE | intenset) |= (1 << source_num);
    *(unsigned char*)(CSR_DMW0_BASE | ioentry + source_num) = ((0x1 << IPx) << 4 | (1 << cpu) << 0);
}

noinstr irqentry_state_t irqentry_enter(struct pt_regs *regs)
{
	irqentry_state_t ret = {
		.exit_rcu = false,
	};

	if (user_mode(regs)) {
		/* 从用户模式进入中断 */
		// irqentry_enter_from_user_mode(regs);
		return ret;
	}
	/* 空闲任务中的中断 */
	// if (is_idle_task(current)) {
	if (false) {
		arch_local_irq_disable();
		
		ret.exit_rcu = true;
		return ret;
	}

	arch_local_irq_disable();
	return ret;
}

noinstr void irqentry_exit(struct pt_regs *regs, irqentry_state_t state)
{
	enum intr_status status = intr_get_status();
	// 中断已禁用
	ASSERT(status == INTR_OFF);

	// 检查是否返回到用户模式
	if (user_mode(regs)) {
		// irqentry_exit_to_user_mode(regs);
		return;
	}

	// 如果中断标志未禁用，启用中断
	arch_local_irq_enable();
}

void irq_init(void) {
    printk("irq_init start\n");
    exception_init();
    printk("irq_init done\n");
}
