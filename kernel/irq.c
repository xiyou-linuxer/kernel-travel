#include <asm/loongarch.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/stdio.h>
#include <trap/irq.h>

#define INTR_NUM	256

intr_handler intr_table[INTR_NUM];
char *intr_name[INTR_NUM];

static void general_intr_handler(struct pt_regs *regs)
{
	unsigned long hwirq = *(unsigned long *)regs;
	unsigned long virq = EXCCODE_INT_START + hwirq;
	printk("!!!!!!!      exception message begin  !!!!!!!!\n");
	printk("intr_table[%x]: %s happened", virq, intr_name[virq]);
	printk("\n!!!!!!!      exception message end    !!!!!!!!\n");
	while(1);
}

static void exception_init(void)
{
	int i;
	for (i = 0; i < INTR_NUM; i++) {
		intr_name[i] = "unknown";
		intr_table[i] = general_intr_handler;
	}
}

enum intr_status intr_get_status(void)
{
	uint64_t crmd;

	crmd = read_csr_crmd();

	return (crmd & CSR_CRMD_IE) ? INTR_ON : INTR_OFF;
}

static inline void arch_local_irq_enable(void)
{
	uint32_t val = CSR_CRMD_IE;
	__asm__ __volatile__(
		"csrxchg %[val], %[mask], %[reg]\n\t"
		: [val] "+r" (val)
		: [mask] "r" (CSR_CRMD_IE), [reg] "i" (LOONGARCH_CSR_CRMD)
		: "memory");
}

static inline void arch_local_irq_disable(void)
{
	uint32_t val = 0;
	__asm__ __volatile__(
		"csrxchg %[val], %[mask], %[reg]\n\t"
		: [val] "+r" (val)
		: [mask] "r" (CSR_CRMD_IE), [reg] "i" (LOONGARCH_CSR_CRMD)
		: "memory");
}

/* 开中断并返回开中断前的状态*/
enum intr_status intr_enable(void)
{
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
enum intr_status intr_disable(void)
{
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
enum intr_status intr_set_status(enum intr_status status)
{
	return status & INTR_ON ? intr_enable() : intr_disable();
}

/* 在中断处理程序数组第vector_no个元素中注册安装中断处理程序function */
void register_handler(uint8_t vector_no, intr_handler function)
{
	intr_table[vector_no] = function;
}

extern void do_irq(struct pt_regs *regs, uint64_t virq);
void do_irq(struct pt_regs *regs, uint64_t virq)
{
	// printk("virq = %d ", virq);
	intr_table[virq](regs);
}

extern void timer_interrupt(struct pt_regs *regs);
void timer_interrupt(struct pt_regs *regs)
{
	printk("timer interrupt\n");
	/* ack */
	write_csr_ticlr(read_csr_ticlr() | (0x1 << 0));
}

void irq_init(void)
{
	printk("irq_init start\n");
	exception_init();
	// register_handler(EXCCODE_TIMER, timer_interrupt);

	printk("irq_init done\n");
}
