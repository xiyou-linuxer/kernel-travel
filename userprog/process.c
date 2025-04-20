#include <process.h>
#include <stdint.h>
#include <xkernel/thread.h>
#include <debug.h>
#include <trap/irq.h>
#include <allocator.h>

#ifdef CONFIG_LOONGARCH
#include <asm/loongarch.h>
#endif

#include <xkernel/stdio.h>
#include <xkernel/memory.h>
#include <xkernel/string.h>
#include <exec.h>
#include <fs/syscall_fs.h>
#include <fs/fd.h>

extern char usrprog[];
extern void user_ret(void);

void create_user_vaddr_bitmap(struct task_struct* user_prog) {
   user_prog->usrprog_vaddr.vaddr_start = USER_VADDR_START;
   uint64_t bitmap_pg_cnt = DIV_ROUND_UP((USER_STACK - USER_VADDR_START) / PAGESIZE / 8 , PAGESIZE) + 1;
   user_prog->usrprog_vaddr.btmp.bits = (uint8_t*)get_pages(bitmap_pg_cnt);
   user_prog->usrprog_vaddr.btmp.btmp_bytes_len = (USER_STACK - USER_VADDR_START) / PAGESIZE / 8;
   uint64_t b = user_prog->usrprog_vaddr.btmp.btmp_bytes_len;
   printk("max address:%llx\n",b*8*PAGESIZE+USER_VADDR_START);
   bitmap_init(&user_prog->usrprog_vaddr.btmp);
}

void start_process(void* filename)
{
#ifdef CONFIG_LOONGARCH
	printk("start process....\n");
	unsigned long crmd;
	unsigned long prmd;
	struct task_struct* cur = running_thread();
	struct pt_regs *regs = (struct pt_regs*)cur->self_kstack;

	regs->csr_crmd = read_csr_crmd();
	prmd = read_csr_prmd() & ~(PLV_MASK);
	prmd |= PLV_USER;
	regs->csr_prmd = prmd;

	malloc_usrpage_withoutopmap(cur->pgdir,USER_TOP-0x2000);
	malloc_usrpage_withoutopmap(cur->pgdir,USER_STACK);
	userstk_alloc(cur->pgdir);
	char *argv[] = {filename,NULL};
	char *envp[] = {NULL};
	sys_execve(filename,argv,envp);
#endif
}

void process_execute(void* filename, char* name,int pri) {
	struct task_struct* pcb = task_alloc();
	init_thread(pcb, name, pri);
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
#ifdef CONFIG_LOONGARCH
	if (pcb->pgdir != 0) {
		write_csr_pgdl(pcb->pgdir);
		invalidate();
	}
	//printk("next pgdir=%x\n",pcb->pgdir);
#endif
}

