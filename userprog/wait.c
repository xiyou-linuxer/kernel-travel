#include <wait.h>
#include <stdint.h>
#include <xkernel/list.h>

static bool init_adopt_child(struct list_elem* child,pid_t pid)
{
    struct task_struct* c = elem2entry(struct task_struct,all_list_tag,child);
    if (c->ppid == pid)
        c->ppid = 1;
    return false;
}

static bool find_hanging_child(struct list_elem* elm,pid_t parent_pid)
{
    struct task_struct* c = elem2entry(struct task_struct,elm,all_list_tag);
    if (c->ppid==parent_pid && c->status==TASK_HANGING) {
        return true;
    }
    return false;
}

static bool find_child(struct list_elem* elm,pid_t parent_pid)
{
    struct task_struct* c = elem2entry(struct task_struct,elm,all_list_tag);
    if (c->ppid==parent_pid) {
        return true;
    }
    return false;
}

static void release_usrprog_resource(struct task_struct* pcb)
{
    uint_32* pdep = pcb->pdir;

    uint_32 vaddr = 0;
    for (int pde_idx = 0; pde_idx < 768; pde_idx++,pdep++)
    {
        if (!(*pdep & PAGE_P)) {
            vaddr += 0x400000;
            continue;
        }

        uint_32* ptep = pte_ptr(vaddr);
        for (int pte_idx = 0; pte_idx < 1024; pte_idx++,ptep++)
        {
            if (!(*ptep & PAGE_P))
                continue;

            pfree(*ptep & 0xfffff000);
        }
        pfree(*pdep & 0xfffff000);
        vaddr += 0x400000;
    }

    uint_32 bitmap_pg_cnt = pcb->usrprog_vaddr.btmp.map_size/PAGESIZE;
    void* btmp = pcb->usrprog_vaddr.btmp.bits;
    mfree_page(PF_KERNEL,btmp,bitmap_pg_cnt);

    for (uint_32 fd = 3 ; fd < MAX_OPEN_FILES_PROC ; fd++) {
        if (pcb->fd_table[fd] != -1) {
            sys_close(fd);
        }
    }
}

void sys_exit(int_32 status)
{
    struct task_struct* child = running_thread();
    child->exit_status = status;
    release_usrprog_resource(child);

    list_traversal(&all_thread_list,init_adopt_child,child->pid);
    struct task_struct* parent = pid2thread(child->ppid);
    if (parent->status == TASK_WAITING) {
        thread_unblock(parent);
    }
    thread_block(TASK_HANGING);
}

pid_t sys_wait(int_32* status)
{
    struct task_struct* parent = running_thread();
    while(1)
    {
        struct list_elem* exit_elm = list_traversal(&all_thread_list,find_hanging_child,parent->pid);
        if (exit_elm != NULL)
        {
            struct task_struct* exit_child = elem2entry(struct task_struct,exit_elm,all_list_tag);
            *status = exit_child->status;
            thread_exit(exit_child,false);
            return exit_child->pid;
        }

        exit_elm = list_traversal(&all_thread_list,find_child,parent->pid);
        if (exit_elm == NULL) {
            return -1;
        }
        thread_block(TASK_WAITING);
    }
}



