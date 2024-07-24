#include <xkernel/types.h>
#include <xkernel/memory.h>
#include <xkernel/compiler.h>
#include <xkernel/irqflags.h>
#include <xkernel/sched.h>
#include <xkernel/thread.h>
#include <asm/pt_regs.h>
#include <asm/loongarch.h>
#include <trap/irq.h>


static void __do_page_fault(struct pt_regs *regs,
			unsigned long write, unsigned long address)
{
	struct task_struct * curr = running_thread();
	struct mm_struct * mm = curr->mm;
	struct vm_area_struct * vma = NULL;
	/* 检查 address 是否在用户空间 */

	/* 检查 address 是否在当前进程的 VMA 中 */

	/* 处理页面错误 */

}

asmlinkage void  do_page_fault(struct pt_regs *regs,
			unsigned long write, unsigned long address)
{
	irqentry_state_t state = irqentry_enter(regs);

	/* 如果中断使能位被设置，则启用中断 */
	if (likely(regs->csr_prmd & CSR_PRMD_PIE))
		local_irq_enable();

	__do_page_fault(regs, write, address);

	local_irq_disable();

	irqentry_exit(regs, state);
	return ;
}
