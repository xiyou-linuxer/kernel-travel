#include <xkernel/wait.h>
#include <stdint.h>
#include <xkernel/list.h>
#include <xkernel/thread.h>
#include <xkernel/stdio.h>
#include <fs/syscall_fs.h>
#include <trap/irq.h>

static int init_adopt_child(struct list_elem* child,void* id)
{
	int64_t pid = (int64_t)id;
	struct task_struct* c = elem2entry(struct task_struct,all_list_tag,child);
	if (c->ppid == pid)
		c->ppid = 1;
	return false;
}

static int find_hanging_child(struct list_elem* elm,void* parent_id)
{
	int64_t ppid = (int64_t)parent_id;
	struct task_struct* c = elem2entry(struct task_struct,all_list_tag,elm);
	if (c->ppid==ppid && c->status==TASK_HANGING) {
		return true;
	}
	return false;
}

static int find_child(struct list_elem* elm,void* parent_pid)
{
	int64_t ppid = (int64_t)parent_pid;
	struct task_struct* c = elem2entry(struct task_struct,all_list_tag,elm);
	if (c->ppid==ppid) {
		return true;
	}
	return false;
}

static void release_usrmemory(uint64_t pdir,struct virt_addr *usrmem)
{
	printk("exit: release %x",pdir);
	uint64_t* pgdp = pgd_ptr(pdir,0);
	uint64_t vaddr = 0;
	for (int pgd_idx = 0; pgd_idx < 512; pgd_idx++,pgdp++)
	{
		if (!*pgdp) {
			vaddr += 0x40000000;
			continue;
		}

		uint64_t* pmdp = pmd_ptr(pdir,vaddr);
		for (int pmd_idx = 0; pmd_idx < 512; pmd_idx++,pmdp++)
		{
			if (!*pmdp) {
				vaddr += 0x200000;
				continue;
			}

			uint64_t* ptep = pte_ptr(pdir,vaddr);
			for (int pte_idx = 0; pte_idx < 512; pte_idx++,ptep++)
			{
				if (!*ptep) {
					continue;
				}
				//printk("exit: release  *ptep %x\n",*ptep&0xfffffffffffff000);
				free_page(*ptep&0xfffffffffffff000);
			}
			//printk("exit: release *pmdp %x\n",*pmdp&0xfffffffffffff000);
			free_page(*pmdp&0xfffffffffffff000);
		}
		//printk("exit: release *pgdp %x\n",*pgdp&0xfffffffffffff000);
		free_page(*pgdp&0xfffffffffffff000);
	}

	uint64_t bitmap_pg_cnt = usrmem->btmp.btmp_bytes_len/PAGESIZE;
	void* btmp = usrmem->btmp.bits;
	free_pages((uint64_t)btmp,bitmap_pg_cnt);
}

static void release_usrprog_resource(struct task_struct* pcb)
{
	release_usrmemory(pcb->pgdir,&pcb->usrprog_vaddr);
	if (pcb->pgdir != 0) {
		free_page(pcb->pgdir);
	}

	//close files
	//for (uint64_t fd = 3 ; fd < MAX_OPEN_FILES_PROC ; fd++) {
	//	if (pcb->fd_table[fd] != -1) {
	//		sys_close(fd);
	//	}
	//}
}

void sys_exit(int status)
{
	struct task_struct* child = running_thread();
	child->exit_status = status;
	release_usrprog_resource(child);

	list_traversal(&thread_all_list,init_adopt_child,(void*)(int64_t)child->pid);
	struct task_struct* parent = pid2thread(child->ppid);
	if (parent->status == TASK_WAITING) {
		thread_unblock(parent);
	}
	for (uint64_t fd = 3 ; fd < MAX_FILES_OPEN_PER_PROC ; fd++) {
		if (child->fd_table[fd] != -1) {
			sys_close(fd);
		}
	}
	thread_block(TASK_HANGING);
}

pid_t sys_wait(pid_t pid,int* status,int options)
{
	struct task_struct* parent = running_thread();
	while(1)
	{
		struct list_elem* exit_elm = list_traversal(&thread_all_list,find_hanging_child,(void*)(int64_t)parent->pid);
		if (exit_elm != NULL)
		{
			struct task_struct* exit_child = elem2entry(struct task_struct,all_list_tag,exit_elm);
			*status = exit_child->exit_status;
			int ret_pid = exit_child->pid;
			thread_exit(exit_child);

			return ret_pid;
		}

		exit_elm = list_traversal(&thread_all_list,find_child,(void*)(int64_t)parent->pid);
		if (exit_elm == NULL) {
			//printk("%s wait end\n",parent->name);
			return -1;
		}
		thread_block(TASK_WAITING);
	}
}

