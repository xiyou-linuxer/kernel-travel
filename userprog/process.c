#include <process.h>
#include <thread.h>
#include <debug.h>
#include <trap/irq.h>
#include <allocator.h>
#include <asm/loongarch.h>
#include <linux/stdio.h>

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
    printk("func addr:%x\n",func);

    crmd = read_csr_crmd() & ~(PLV_MASK);
    crmd |= PLV_USER;
    regs->csr_crmd = crmd;

    prmd = read_csr_prmd() & ~(PLV_MASK);
    prmd |= PLV_USER;
    regs->csr_prmd = prmd;
    printk("prmd:%x,crmd:%x\n",prmd,crmd);

    //84ca
    //register uint64_t sp asm("sp");
    //printk("now sp=")
    regs->regs[3] = (uint64_t)userstk_alloc();
    printk("%x\n",regs->regs[3]);

    asm volatile("addi.d $r3,%0,0;b user_ret;"::"g"((uint64_t)regs):"memory");
}

void process_execute(void* filename, char* name) { 
   struct task_struct* pcb = task_alloc();
   init_thread(pcb, name, 31);
   //create_user_vaddr_bitmap(thread);
   thread_create(pcb, start_process, filename);
   //pcb->pgdir = create_page_dir();

   enum intr_status old_status = intr_disable();
   ASSERT(!elem_find(&thread_ready_list, &pcb->general_tag));
   list_append(&thread_ready_list, &pcb->general_tag);

   ASSERT(!elem_find(&thread_all_list, &pcb->all_list_tag));
   list_append(&thread_all_list, &pcb->all_list_tag);
   intr_set_status(old_status);
}

