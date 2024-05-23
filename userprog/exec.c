#include <exec.h>
#include <xkernel/memory.h>
#include <xkernel/thread.h>
#include <xkernel/stdio.h>
#include <stdint.h>
#include <xkernel/string.h>
#include <asm/loongarch.h>
#include <fs/syscall_fs.h>
#include <fs/fd.h>
#include <xkernel/mmap.h>
#include <xkernel/types.h>
#include <allocator.h>
#include <trap/irq.h>

void* program_begin;

void pro_seek(void **program,int offset)
{
	char** p = (char**)program;
	*p = (char*)program_begin + offset;
}

void pro_read(void *program,void *buf,uint64_t count)
{
	memcpy(buf,program,count);
}

static bool load_phdr(uint32_t fd,Elf_Phdr *phdr)
{
	uint64_t page_cnt;
	int64_t remain_size;
	remain_size = phdr->p_filesz - (PAGESIZE - (phdr->p_vaddr&0xfff));
	if (remain_size > 0) {
		page_cnt = DIV_ROUND_UP(remain_size,PAGESIZE) + 1;
	} else {
		page_cnt = 1;
	}
	/* 这里 pd 为 0 导致错误*/
	uint64_t pd = running_thread()->pgdir;
	uint64_t pg_start = phdr->p_vaddr&0xfffffffffffff000;
	for (uint64_t p = 0 ,page = pg_start ; p < page_cnt ; p++)
	{
		uint64_t* pte = pte_ptr(pd,page);
		if (*pte == 0)
			malloc_usrpage(pd,page);
		//printk("load_phdr:cur->vaddrbtmp=%d\n",*(unsigned long*)running_thread()->usrprog_vaddr.btmp.bits);

		page += PAGESIZE;
	}

	sys_lseek(fd,phdr->p_offset,SEEK_SET);
	sys_read(fd,(void*)phdr->p_vaddr,phdr->p_filesz);
	return true;
}

static unsigned long elf_map(struct file *filep, unsigned long addr,Elf_Phdr *eppnt, int prot, int type)
{
	unsigned long map_addr;

	// down_write(&current->mm->mmap_sem);
	map_addr = do_mmap(filep, ELF_PAGESTART(addr),
			   eppnt->p_filesz + ELF_PAGEOFFSET(eppnt->p_vaddr), prot, type,
			   eppnt->p_offset - ELF_PAGEOFFSET(eppnt->p_vaddr));
	// up_write(&current->mm->mmap_sem);
	return(map_addr);
}

static inline void arch_pick_mmap_layout(struct mm_struct *mm)
{
	mm->mmap_base = TASK_UNMAPPED_BASE;
	mm->get_unmapped_area = get_unmapped_area;
	// mm->unmap_area = arch_unmap_area;
}

// int setup_arg_pages(struct linux_binprm *bprm,
int setup_arg_pages(void *bprm,
		    unsigned long stack_top,
		    int executable_stack)
{
	// unsigned long stack_base;
	// long arg_size;
	// struct mm_struct *mm = running_thread()->mm;
	// struct vm_area_struct * vm_area;
	// stack_base = PAGE_ALIGN(arch_align_stack(stack_top - MAX_ARG_PAGES*PAGE_SIZE));
	// mm->arg_start = stack_base;
	// arg_size = stack_top - (PAGE_MASK & (unsigned long) mm->arg_start);
	// vm_area = (struct vm_area_struct *)get_page();
	// vm_area->vm_end = stack_top;
	// vm_area->vm_start = vm_area->vm_end - arg_size;
	return -1;
}

int64_t load(const char *path)
{
	Elf_Ehdr ehdr;
	memset(&ehdr,0,sizeof(ehdr));
	int fd = sys_open(path, O_RDWR ,660);
	sys_lseek(fd,0,SEEK_SET);
	int size = sys_read(fd, &ehdr, sizeof(ehdr));

	int64_t ret;
	if (memcmp(ehdr.e_ident,"\177ELF",4) || \
		ehdr.e_ident[4] != 2 || \
		ehdr.e_ident[5] != 1 || \
		ehdr.e_ident[6] != 1)
	{
		printk("load file failed\n");
		ret = -1;
		goto done;
	}
	/* mm_strct 初始化*/
	// running_thread()->mm->start_data = 0;
	// running_thread()->mm->end_data = 0;
	
	// running_thread()->mm->end_code = 0;
	// running_thread()->mm->mmap = NULL;
	// running_thread()->mm->rss = 0;

	/*获取当前内存布局*/
	arch_pick_mmap_layout(running_thread()->mm);
	running_thread()->mm->free_area_cache = running_thread()->mm->mmap_base;
	/*设置 stack 的 vm_area_struct*/
	// setup_arg_pages(bprm, randomize_stack_top(STACK_TOP),
	// 			 executable_stack);

	Elf_Phdr phdr;
	uint64_t phoff = ehdr.e_phoff;
	for (uint64_t ph = 0 ; ph < ehdr.e_phnum ; ph++)
	{
		int elf_prot = 0, elf_flags = 0;
		unsigned long v_addr = 0;

		memset(&phdr,0,sizeof(phdr));
		sys_lseek(fd,phoff,SEEK_SET);
		sys_read(fd,&phdr,sizeof(phdr));
		if (phdr.p_type == PT_LOAD) {
			if (phdr.p_flags & PF_R) elf_prot |= PROT_READ;
			if (phdr.p_flags & PF_W) elf_prot |= PROT_WRITE;
			if (phdr.p_flags & PF_X) elf_prot |= PROT_EXEC;
			elf_flags = MAP_PRIVATE|MAP_DENYWRITE|MAP_EXECUTABLE;
			v_addr = phdr.p_vaddr;
			load_phdr(fd,&phdr);
			/*初始化 vm_area_struct*/
			// elf_map(NULL, v_addr, &phdr,elf_prot, elf_flags);
		}
		phoff += ehdr.e_phentsize;
	}

	ret = ehdr.e_entry;
done:
	return ret;
}


int sys_exeload(const char *path, char *const argv[], char *const envp[])
{
	int64_t entry_point = load(path);
	if (entry_point == -1) {
		printk("sys_exeload: load failed\n");
		return -1;
	}

	return entry_point;
}

int sys_execve(const char *path, char *const argv[], char *const envp[])
{
	unsigned long crmd;
	unsigned long prmd;
	struct task_struct* cur = running_thread();
	//printk("%s sys_execve\n",cur->name);
	strcpy(cur->name,path);
	struct pt_regs* regs = (struct pt_regs*)((uint64_t)cur->self_kstack - sizeof(struct pt_regs));

	regs->csr_crmd = read_csr_crmd();
	prmd = read_csr_prmd() & ~(PLV_MASK);
	prmd |= PLV_USER;
	regs->csr_prmd = prmd;

	regs->regs[3]  = 1 << (9+9+12);
	regs->regs[22] = regs->regs[3];
	int64_t entry = sys_exeload(path,NULL,NULL);
	regs->csr_era = (unsigned long)entry;

	//intr_enable();
	//printk("jump to proc... at %d\n",entry);
	asm volatile("addi.d $r3,%0,0;b user_ret;"::"g"((uint64_t)regs):"memory");
	return -1;
}

