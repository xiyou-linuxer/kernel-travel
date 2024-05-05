#include <fork.h>
#include <linux/stdio.h>
#include <linux/memory.h>
#include <linux/thread.h>
#include <debug.h>
#include <string.h>

static int copy_pcb(struct task_struct* parent,struct task_struct* child)
{
	memcpy(child,parent,PAGESIZE);
	child->self_kstack = (uint64_t*)((uint64_t)child + PAGESIZE);
	child->pid = fork_pid();
	child->ticks    = parent->priority;
	child->elapsed_ticks = 0;
	child->status   = TASK_READY;
	child->all_list_tag.prev = child->all_list_tag.next = NULL;
	child->general_tag.prev = child->general_tag.next = NULL;
	//block_desc_init(child->usr_block_desc);

	//uint64_t vaddr_btmp_size = DIV_ROUND_UP((0xc0000000 - USR_VADDR_START)/PAGESIZE/8,PAGESIZE);
	//child->usrprog_vaddr.btmp.bits = get_kernel_pages(vaddr_btmp_size);
	//if (child->usrprog_vaddr.btmp.bits == NULL) {
	//	printk("copy_pcb: usrprog_vaddr bitmap.bits malloc failed\n");
	//	return -1;
	//}
	//memcpy(child->usrprog_vaddr.btmp.bits,  \
	//	   parent->usrprog_vaddr.btmp.bits, \
	//	   parent->usrprog_vaddr.btmp.map_size);

	strcat(child->name,"_fork");
	return 0;
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
	//if (copy_body_stack3(parent,child,page) == -1) {
	//	return -1;
	//}

	//make_switch_prepare(child);

	//update_inode_openstat(child);

	//mfree_page(PF_KERNEL,page,1);
	return 0;
}

pid_t sys_fork(void)
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
