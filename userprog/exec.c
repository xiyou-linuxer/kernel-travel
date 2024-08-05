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
#include <asm/timer.h>
#include "asm/page.h"
#include "sync.h"

void* program_begin;

void append_to_auxv(Elf_auxv_t *auxv,uint64_t arg_vec[])
{
	auxv->a_type = arg_vec[0];
	auxv->a_val  = arg_vec[1];
}

void pro_seek(void **program,int offset)
{
	char** p = (char**)program;
	*p = (char*)program_begin + offset;
}

void pro_read(void *program,void *buf,uint64_t count)
{
	memcpy(buf,program,count);
}

/* 给定的程序头将一个可执行文件的段加载到虚拟内存中 */
static bool load_phdr(uint32_t fd,Elf_Phdr *phdr)
{
	uint64_t page_cnt;
	int64_t remain_size;
	remain_size = phdr->p_memsz - (PAGESIZE - (phdr->p_vaddr&0xfff));
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
		else {
			*pte = 0;
			malloc_usrpage(pd,page);
		}

		page += PAGESIZE;
	}
	sys_lseek(fd,phdr->p_offset,SEEK_SET);
	printk("load_phdr phdr->p_filesz:%d\n",phdr->p_filesz);
	sys_read(fd,(void*)phdr->p_vaddr,phdr->p_filesz);
	return true;
}

static unsigned long elf_map(struct file *filep, unsigned long addr,Elf_Phdr *eppnt, int prot, int type)
{

	unsigned long map_addr;

	sema_down(&running_thread()->mm->map_lock);

	map_addr = do_mmap(filep, ELF_PAGESTART(addr),
			   eppnt->p_filesz + ELF_PAGEOFFSET(eppnt->p_vaddr), prot, type,
			   eppnt->p_offset - ELF_PAGEOFFSET(eppnt->p_vaddr));

	printk("map_addr: 0x%lx\n",map_addr);
	printk("map_prot : %x\n",prot);

	sema_up(&running_thread()->mm->map_lock);

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

int64_t load(const char *path,Elf_Ehdr *ehdr,uint64_t *phaddr)
{
	int fd = sys_open(path, O_RDWR ,660);
	sys_lseek(fd,0,SEEK_SET);
	int size = sys_read(fd, ehdr, sizeof(*ehdr));
	printk("name %s\n",path);
	int64_t ret;
	if (memcmp(ehdr->e_ident,"\177ELF",4) || \
		ehdr->e_ident[4] != 2 || \
		ehdr->e_ident[5] != 1 || \
		ehdr->e_ident[6] != 1)
	{
		printk("load file failed\n");
		ret = -1;
		goto done;
	}
	/* mm_strct 初始化*/
	running_thread()->mm->start_data = 0;
	running_thread()->mm->end_data = 0;
	
	running_thread()->mm->end_code = 0;
	running_thread()->mm->mmap = NULL;
	running_thread()->mm->rss = 0;

	/*获取当前内存布局*/
	arch_pick_mmap_layout(running_thread()->mm);
	running_thread()->mm->free_area_cache = running_thread()->mm->mmap_base;
	/*设置 stack 的 vm_area_struct*/
	// setup_arg_pages(bprm, randomize_stack_top(STACK_TOP),
	// 			 executable_stack);

	Elf_Phdr phdr;
	uint64_t phoff = ehdr->e_phoff;
	struct mm_struct * mm = running_thread()->mm;

	for (uint64_t ph = 0 ; ph < ehdr->e_phnum ; ph++)
	{
		int elf_prot = 0, elf_flags = 0;
		unsigned long v_addr = 0;

		memset(&phdr,0,sizeof(phdr));
		sys_lseek(fd,phoff,SEEK_SET);
		sys_read(fd,&phdr,sizeof(phdr));
		if (phdr.p_type == PT_LOAD) {
			/* 获取程序头表的虚拟地址 */
			if (phdr.p_offset <= phoff && phoff < phdr.p_offset+phdr.p_filesz)
				*phaddr = phdr.p_vaddr+(phoff-phdr.p_offset);

			if (phdr.p_flags & PF_R) elf_prot |= PROT_READ;
			if (phdr.p_flags & PF_W) elf_prot |= PROT_WRITE;
			if (phdr.p_flags & PF_X) elf_prot |= PROT_EXEC;
			elf_flags = MAP_PRIVATE|MAP_DENYWRITE|
					MAP_EXECUTABLE|MAP_FOR_INIT_FILE;
			v_addr = phdr.p_vaddr;
			//printk("vaddr=%llx:p_offset=%llx\n:filesz=%llx\n",phdr.p_vaddr,phdr.p_offset,phdr.p_filesz);
			printk("range:%llx - %llx\n",phdr.p_vaddr,phdr.p_vaddr+phdr.p_filesz);
			load_phdr(fd,&phdr);
			/*初始化 vm_area_struct*/
			unsigned long addr = 
				elf_map(NULL, v_addr, &phdr,elf_prot, elf_flags);
			/* 代码段和数据段的 mm_struct 数据维护*/
			if ((addr | (elf_flags & MAP_FOR_INIT_FILE)) && elf_prot) {
				if (elf_prot & (PROT_EXEC|PROT_READ)) {
					mm->start_code = addr;
					mm->end_code = addr + phdr.p_filesz + ELF_PAGEOFFSET(phdr.p_vaddr);
				}
				/*init进程的code和data在一个程序头，因此分开使用两个if*/
				if (elf_prot & (PROT_READ|PROT_WRITE)) {
					mm->start_data = addr;
					mm->end_data = addr + phdr.p_filesz + ELF_PAGEOFFSET(phdr.p_vaddr);
				}
			}
		}
		phoff += ehdr->e_phentsize;
	}
	/* 进程的 stack 默认 */
	/*实现一个 ramdom 函数用来增强安全性
		目前思路：龙芯cpu有个倒计时时钟，准备写一个读取计时器数值
			   并且限制取值范围的函数用于随机化
	*/
	mm->start_stack = USER_STACK;
	mm->start_brk = HEAP_START;
	mm->brk = mm->start_brk;
	mm->arg_start = USER_TOP - 0x2000;
	mm->arg_end = USER_TOP;

	sema_down(&mm->map_lock);

	do_mmap(NULL, mm->start_stack - PAGE_SIZE, PAGE_SIZE, 
		VM_READ | VM_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS|MAP_FOR_INIT_FILE, 0);
	do_mmap(NULL, mm->start_brk, HEAP_LENGTH, VM_READ | VM_WRITE | VM_EXEC,
 		MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS|MAP_FOR_INIT_FILE, 0);

	do_mmap(NULL, mm->arg_start,mm->arg_end - mm->arg_start, 
		VM_READ | VM_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS|MAP_FOR_INIT_FILE, 0);

	sema_down(&mm->map_lock);
	

	ret = ehdr->e_entry;
done:
	return ret;
}


int64_t sys_exeload(const char *path,Elf_Ehdr *ehdr,uint64_t *phaddr)
{
	int64_t entry_point = load(path,ehdr,phaddr);
	if (entry_point == -1) {
		printk("sys_exeload: load failed\n");
		return -1;
	}

	return entry_point;
}

int sys_execve(const char *path, char *const argv[], char *const envp[])
{
	uint64_t argc = 0, envs = 0, auxs = 13;
	uint64_t phaddr;
	Elf_Ehdr ehdr;
	memset(&ehdr,0,sizeof(ehdr));
	while (argv[argc]) argc++;
	if (envp != NULL)
		while (envp[envs]) envs++;

	unsigned long crmd;
	unsigned long prmd;
	struct task_struct* cur = running_thread();
	//printk("%s sys_execve\n",cur->name);
	strcpy(cur->name,path);
	struct pt_regs* regs = (struct pt_regs*)((uint64_t)cur + PAGESIZE -sizeof(struct pt_regs));

	regs->csr_crmd = read_csr_crmd();
	prmd = read_csr_prmd() & ~(PLV_MASK);
	prmd |= PLV_USER;
	regs->csr_prmd = prmd;

	userstk_alloc(cur->pgdir);
	regs->regs[3]  = USER_STACK-24 - (argc+envs+3)*(sizeof(uint64_t)) - auxs*sizeof(Elf_auxv_t);
	regs->regs[22] = regs->regs[3];
	regs->regs[4] = regs->regs[3];

	char (*uargs) [20] = (char (*)[20])USER_STACK;
	malloc_usrpage(cur->pgdir,(uint64_t)uargs);
	struct mm_struct* mm = (struct mm_struct *)get_page();
	cur->mm = mm;
	mm_struct_init(cur->mm);
	int64_t entry = sys_exeload(path,&ehdr,&phaddr);
	regs->csr_era = (unsigned long)entry;

	/* random */
	uint64_t random = USER_STACK-24;
	for (int i = 0; i < 16; i++)
		*(char*)(random+i) = i;

	/* envp 转移参数 */
	Elf_auxv_t *auxv = (Elf_auxv_t *)(random - auxs*sizeof(Elf_auxv_t));
	uint64_t argtop = (uint64_t)auxv;
	for (int i = 0; i < envs; i++) {
		strcpy(uargs[argc+i],envp[i]);
	}

	/* argv 转移参数 */
	for (int i = 0; i < argc; i++) {
		strcpy(uargs[i],argv[i]);
	}


	/*   下面是对栈中内容的替换操作   */
	*((uint64_t*)(USER_STACK - sizeof(uint64_t))) = 0;
	/* auxv */
	append_to_auxv(auxv+auxs-1, (uint64_t[2]){AT_NULL,AT_NULL});
	append_to_auxv(auxv+auxs-2, (uint64_t[2]){AT_HWCAP,0});
	append_to_auxv(auxv+auxs-3, (uint64_t[2]){AT_PAGESZ,PAGESIZE});
	append_to_auxv(auxv+auxs-4, (uint64_t[2]){AT_PHDR,phaddr});
	append_to_auxv(auxv+auxs-5, (uint64_t[2]){AT_PHENT,ehdr.e_phentsize});
	append_to_auxv(auxv+auxs-6, (uint64_t[2]){AT_PHNUM,ehdr.e_phnum});
	append_to_auxv(auxv+auxs-7, (uint64_t[2]){AT_UID,0});
	append_to_auxv(auxv+auxs-8, (uint64_t[2]){AT_EUID,0});
	append_to_auxv(auxv+auxs-9, (uint64_t[2]){AT_GID,0});
	append_to_auxv(auxv+auxs-10,(uint64_t[2]){AT_ENTRY,ehdr.e_entry});
	append_to_auxv(auxv+auxs-11,(uint64_t[2]){AT_SECURE,0});
	append_to_auxv(auxv+auxs-12,(uint64_t[2]){AT_RANDOM,random});
	append_to_auxv(auxv+auxs-13,(uint64_t[2]){AT_EXECFD,(uint64_t)uargs[0]});

	/* envp */
	for (int i = 0; i < envs; i++) {
		*((uint64_t*)(argtop - (envs+1-i)*sizeof(uint64_t))) = (uint64_t)uargs[argc+i];
	}
	*((uint64_t*)(argtop - sizeof(uint64_t))) = 0;

	/* argv */
	for (int i = 0; i < argc; i++) {
		*((uint64_t*)(argtop - (envs+argc+2-i)*sizeof(uint64_t))) = (uint64_t)uargs[i];
	}
	*((uint64_t*)(argtop - (envs+2)*sizeof(uint64_t))) = 0;

	/* argc */
	*((uint64_t*)(argtop - (envs+argc+3)*sizeof(uint64_t))) = argc;

	//intr_enable();
	
	/* test arguments */
	uint64_t stk = regs->regs[3];
	printk("USER_STACK:%llx\n",USER_STACK);
	printk("stack:%llx\n",regs->regs[3]);
	printk("argc:%d\n",*(uint64_t*)stk);
	for (int i = 1; i <= argc+1; i++)
		printk("argv[%d]:%s\n",i-1,(char*)*(uint64_t*)(stk+i*sizeof(uint64_t)));
	for (int i = 0; i <= envs; i++)
		printk("envp[%d]:%s\n",i,(char*)*(uint64_t*)(stk+(argc+2+i)*sizeof(uint64_t)));
	for (int i = 0; i < 2*auxs; i+=2) {
		printk("address at:%llx\n",auxv+i/2);
		printk("auxv[%d]:%d,auxv[%d]:%llx\n",i, \
			   *(uint64_t*)(stk+(argc+envs+3+i)*sizeof(uint64_t)), \
			   i+1,*(uint64_t*)(stk+(argc+envs+3+i+1)*sizeof(uint64_t)));
	}
	printk("random at:%llx\n",random);
	for (int i = 0; i < 16; i++)
		printk("random[%d]:%d\n",i,*(char*)(random+i));


	printk("jump to proc... at %llx\n",entry);
	utimes_begin(cur);
	asm volatile("addi.d $r3,%0,0;b user_ret;"::"g"((uint64_t)regs):"memory");
	return -1;
}

