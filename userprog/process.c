#include <process.h>
#include <xkernel/thread.h>
#include <debug.h>
#include <trap/irq.h>
#include <allocator.h>
#include <asm/loongarch.h>
#include <xkernel/stdio.h>
#include <xkernel/memory.h>
#include <xkernel/string.h>
#include <exec.h>

extern char usrprog[];
extern void user_ret(void);

void create_user_vaddr_bitmap(struct task_struct* user_prog) {
   user_prog->usrprog_vaddr.vaddr_start = USER_VADDR_START;
   uint32_t bitmap_pg_cnt = DIV_ROUND_UP((USER_STACK - USER_VADDR_START) / PAGESIZE / 8 , PAGESIZE);
   user_prog->usrprog_vaddr.btmp.bits = (uint8_t*)get_pages(bitmap_pg_cnt);
   user_prog->usrprog_vaddr.btmp.btmp_bytes_len = (USER_STACK - USER_VADDR_START) / PAGESIZE / 8;
   bitmap_init(&user_prog->usrprog_vaddr.btmp);
}


void start_process(void* filename)
{
	printk("start process....\n");
	unsigned long crmd;
	unsigned long prmd;
	void* func = filename;
	struct task_struct* cur = running_thread();
	struct pt_regs *regs = (struct pt_regs*)cur->self_kstack;
	regs->csr_era = (unsigned long)func;

	regs->csr_crmd = read_csr_crmd();
	prmd = read_csr_prmd() & ~(PLV_MASK);
	prmd |= PLV_USER;
	regs->csr_prmd = prmd;

	regs->regs[3] = (uint64_t)userstk_alloc(cur->pgdir);
	regs->regs[22] = regs->regs[3];

	int entry = sys_execv(filename);
	regs->csr_era = (unsigned long)entry;

	printk("jump to proc...\n");
	asm volatile("addi.d $r3,%0,0;b user_ret;"::"g"((uint64_t)regs):"memory");
}

void process_execute(void* filename, char* name) {
   struct task_struct* pcb = task_alloc();
   init_thread(pcb, name, 10);
   create_user_vaddr_bitmap(pcb);
   thread_create(pcb, start_process, filename);
   pcb->pgdir = get_page();
   enum intr_status old_status = intr_disable();
   ASSERT(!elem_find(&thread_ready_list, &pcb->general_tag));
   list_append(&thread_ready_list, &pcb->general_tag);

   ASSERT(!elem_find(&thread_all_list, &pcb->all_list_tag));
   list_append(&thread_all_list, &pcb->all_list_tag);
   intr_set_status(old_status);
}


void page_dir_activate(struct task_struct* pcb)
{
	if (pcb->pgdir != 0) {
		write_csr_pgdl(pcb->pgdir);
		invalidate();
	}
}

