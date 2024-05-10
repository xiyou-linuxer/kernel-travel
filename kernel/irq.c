#include <asm/loongarch.h>
#include <asm/timer.h>
#include <linux/debug.h>
#include <linux/printk.h>
#include <linux/stdio.h>
#include <linux/types.h>
#include <trap/irq.h>
#include <trap/softirq.h>

#define INTR_NUM 256

uint32_t irq_count = 0;
uint32_t bh_count = 0;

intr_handler intr_table[INTR_NUM];
char* intr_name[INTR_NUM];

static void irq_enter(void) {
    ++irq_count;
}
static void irq_exit(void) {
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
	//raise_softirq(TIMER_SOFTIRQ);
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

void irq_init(void) {
    printk("irq_init start\n");
    exception_init();
    printk("irq_init done\n");
}
