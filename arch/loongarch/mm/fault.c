#include <xkernel/types.h>
#include <xkernel/memory.h>
#include <xkernel/compiler.h>
#include <xkernel/irqflags.h>
#include <xkernel/sched.h>
#include <xkernel/thread.h>
#include <xkernel/mmap.h>
#include <asm/pt_regs.h>
#include <asm/loongarch.h>
#include <trap/irq.h>
#include <sync.h>

vm_fault_t handle_mm_fault(struct vm_area_struct *vma, unsigned long address,
                           unsigned int flags, struct pt_regs *regs)
{
	vm_fault_t ret = 0;
	struct task_struct *curr = running_thread();
	struct mm_struct *mm = vma->vm_mm;
	u64 *pgd;
	u64 *pmd;
	u64 *pte;

	// 获取 PGD
	pgd = pgd_ptr(curr->pgdir, address);
	if (unlikely(!pgd)) {
		return VM_FAULT_OOM;
	}

    // 获取 PMD
	pmd = pmd_ptr(curr->pgdir, address);
	if (unlikely(!pmd)) {
		return VM_FAULT_OOM;
	}

	// 获取 PTE
	pte = pte_ptr(curr->pgdir, address);
	if (unlikely(!pte)) {
		return VM_FAULT_OOM;
	}

	// 更新统计信息 (假设有相应的实现)
	// mm_account_fault(regs, address, flags, ret);

	// 调用页表项故障处理函数 (需要实现)
	// ret = handle_pte_fault(mm, vma, address, pte, pmd, flags);

    return ret;
}


static void __do_page_fault(struct pt_regs *regs,
			unsigned long write, unsigned long address)
{
	struct task_struct * curr = running_thread();
	struct mm_struct * mm = curr->mm;
	struct vm_area_struct * vma = NULL;
	unsigned int flags = 0;
	vm_fault_t fault;


	/* 检查 address 在内核空间 */
	if (address & __UA_LIMIT) {
		/* 添加 no_context，当内核发生 Page Fault*/
		return;
	}
	/* 检查 address 是否在当前进程的 VMA 中 */
retry:
	sema_down(&mm->map_lock);
	vma = find_vma(mm, address);

	if (!vma)
		goto bad_area;
	
	if(vma->vm_start <= address) {
		goto good_area;
	}



	/*进程访问的非法虚拟地址*/
bad_area:
	sema_up(&mm->map_lock);
	return;

	/*合法虚拟地址*/
good_area:

	if (write) {
		flags |= FAULT_FLAG_WRITE;
		if (!(vma->vm_flags & VM_WRITE))
			goto bad_area;
	} else {
		/*检查虚拟内存区域是否具有读权限*/
		if (!(vma->vm_flags & VM_READ))
			goto bad_area;
		/*如果是执行操作，检查虚拟内存区域是否具有执行权限*/
		if (!(vma->vm_flags & VM_EXEC))
			goto bad_area;
	}

	/* 处理页面错误 */
	fault = handle_mm_fault(vma, address, flags, regs);

	sema_up(&mm->map_lock);
	return;

}


/*
 参数1 ： struct pt_regs *regs
	指向当前 CPU 寄存器状态的结构体。
 参数2 ：unsigned long write
	指示导致缺页异常的操作类型。
	0 表示缺页异常是由于读取内存引起的（读操作）。
	1 表示缺页异常是由于写入内存引起的（写操作）。
	帮助处理程序决定如何处理缺页，例如是否需要分配新页面。
 参数3 ：unsigned long address
	发生缺页异常的虚拟地址。

 使用堆栈传残
*/
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
