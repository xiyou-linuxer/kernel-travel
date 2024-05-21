#include <fork.h>
#include <xkernel/stdio.h>
#include <xkernel/memory.h>
#include <xkernel/thread.h>
#include <debug.h>
#include <xkernel/string.h>
#include <process.h>
#include <asm/pt_regs.h>
#include <xkernel/switch.h>
#include <asm/loongarch.h>

extern void user_ret(void);

static int copy_pcb(struct task_struct* parent,struct task_struct* child)
{
	memcpy(child,parent,PAGESIZE);
	child->self_kstack = (uint64_t*)((uint64_t)child + PAGESIZE);
	child->pid  = fork_pid();
	child->ppid = parent->pid;
	child->ticks    = parent->priority;
	child->elapsed_ticks = 0;
	child->status   = TASK_READY;
	child->all_list_tag.prev = child->all_list_tag.next = NULL;
	child->general_tag.prev = child->general_tag.next = NULL;
	//block_desc_init(child->usr_block_desc);

	uint64_t vaddr_btmp_size = DIV_ROUND_UP((USER_STACK - USER_VADDR_START)/PAGESIZE/8,PAGESIZE);
	child->usrprog_vaddr.btmp.bits = (uint8_t*)get_pages(vaddr_btmp_size);
	child->usrprog_vaddr.btmp.btmp_bytes_len = parent->usrprog_vaddr.btmp.btmp_bytes_len;
	if (child->usrprog_vaddr.btmp.bits == NULL) {
		printk("copy_pcb: usrprog_vaddr bitmap.bits malloc failed\n");
		return -1;
	}
	memcpy(child->usrprog_vaddr.btmp.bits,  \
		   parent->usrprog_vaddr.btmp.bits, \
		   parent->usrprog_vaddr.btmp.btmp_bytes_len);

	strcat(child->name,"_fork");
	return 0;
}

static int64_t copy_body_stack3(struct task_struct* parent,struct task_struct* child,void* page)
{
	uint64_t vaddr_start = child->usrprog_vaddr.vaddr_start;
	struct bitmap* btmp = &child->usrprog_vaddr.btmp;
	uint64_t end_byte = btmp->btmp_bytes_len;
	for (uint64_t b = 0 ; b < end_byte ; b++)
	{
		if (btmp->bits[b] == 0) continue;
		uint64_t bit_idx = 0;
		while (bit_idx < 8)
		{
			if ((btmp->bits[b] & (1<<bit_idx)) == 0) {
				bit_idx++;
				continue;
			}
			uint64_t vaddr = (b*8+bit_idx)*PAGESIZE + vaddr_start;
			memcpy(page,(void*)vaddr,PAGESIZE);
			page_dir_activate(child);
			malloc_usrpage_withoutopmap(child->pgdir,vaddr);
			memcpy((void*)vaddr,page,PAGESIZE);
			page_dir_activate(parent);
			bit_idx++;
		}
	}
	return 0;
}

static void make_switch_prepare(struct task_struct* child)
{
	struct pt_regs* regs = (struct pt_regs*)((uint64_t)child->self_kstack - sizeof(struct pt_regs));
	//child->self_kstack = (uint64_t*)((uint64_t)child->self_kstack - sizeof(struct pt_regs));

	regs->regs[4] = 0;
	prepare_switch(child,(uint64_t)user_ret,(uint64_t)regs);
}


static int copy_process(struct task_struct* parent,struct task_struct* child)
{
	void* page = (void*)get_page();
	if (page == NULL) {
		printk("copy_process: page malloc failed\n");
		return -1;
	}

	if (copy_pcb(parent,child) == -1) {
		return -1;
	}

	child->pgdir = get_page();
	if (copy_body_stack3(parent,child,page) == -1) {
		return -1;
	}

	make_switch_prepare(child);

	//update_inode_openstat(child);

	//mfree_page(PF_KERNEL,page,1);
	return 0;
}

pid_t sys_fork(uint64_t flag,int stack)
{
	struct task_struct* cur = running_thread();
	struct task_struct* child = (struct task_struct*)get_page();
	if (child == NULL) {
		printk("sys_fork: child get_kernel_pages failed\n");
		return -1;
	}

	if (copy_process(cur,child) == -1) {
		printk("sys_fork: copy_process failed\n");
		return -1;
	}

	ASSERT(!elem_find(&thread_all_list,&child->all_list_tag));
	list_append(&thread_all_list,&child->all_list_tag);
	ASSERT(!elem_find(&thread_ready_list,&child->general_tag));
	list_append(&thread_ready_list,&child->general_tag);

	return child->pid;
}
