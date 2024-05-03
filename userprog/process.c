#include <process.h>
#include <linux/thread.h>
#include <debug.h>
#include <trap/irq.h>
#include <allocator.h>
#include <asm/loongarch.h>
#include <linux/stdio.h>
#include <linux/memory.h>
#include <linux/string.h>


char proc0_code[] = {0x00, 0x00, 0x00, 0x50};
extern void user_ret(void);

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

    uint64_t page = get_page();
    memcpy((void*)page,func,128);
    page_table_add(cur->pgdir,0,page&~DMW_MASK,PTE_V | PTE_PLV | PTE_D);
    regs->csr_era = 0;

    printk("jump to proc...\n");
    asm volatile("addi.d $r3,%0,0;b user_ret;"::"g"((uint64_t)regs):"memory");
}

void process_execute(void* filename, char* name) { 
   struct task_struct* pcb = task_alloc();
   init_thread(pcb, name, 31);
   //create_user_vaddr_bitmap(thread);
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
    }
}

