#include <xkernel/types.h>
#include <xkernel/memory.h>
#include <xkernel/compiler.h>
#include <xkernel/irqflags.h>
#include <asm/pt_regs.h>
#include <trap/irq.h>



asmlinkage void  do_page_fault(struct pt_regs *regs,
			unsigned long write, unsigned long address)
{
	irqentry_state_t state = irqentry_enter(regs);

	/* 如果中断使能位被设置，则启用中断 */
	if (likely(regs->csr_prmd & CSR_PRMD_PIE))
		local_irq_enable();

	// __do_page_fault(regs, write, address);

	local_irq_disable();

	// irqentry_exit(regs, state);
	return ;
}
