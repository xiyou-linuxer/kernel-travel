#include <exec.h>
#include <xkernel/memory.h>
#include <xkernel/thread.h>
#include <xkernel/stdio.h>
#include <stdint.h>
#include <xkernel/string.h>
#include <asm/loongarch.h>
#include <fs/syscall_fs.h>
#include <fs/fd.h>
#include <allocator.h>

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

	uint64_t pd = running_thread()->pgdir;
	uint64_t pg_start = phdr->p_vaddr&0xfffffffffffff000;
	for (uint64_t p = 0 ,page = pg_start ; p < page_cnt ; p++)
	{
		uint64_t* pte = pte_ptr(pd,page);
		if (*pte == 0)
			malloc_usrpage(pd,page);
		printk("load_phdr:cur->vaddrbtmp=%d\n",*(unsigned long*)running_thread()->usrprog_vaddr.btmp.bits);

		page += PAGESIZE;
	}

	sys_lseek(fd,phdr->p_offset,SEEK_SET);
	sys_read(fd,(void*)phdr->p_vaddr,phdr->p_filesz);
	return true;
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

	Elf_Phdr phdr;
	uint64_t phoff = ehdr.e_phoff;
	for (uint64_t ph = 0 ; ph < ehdr.e_phnum ; ph++)
	{
		memset(&phdr,0,sizeof(phdr));
		sys_lseek(fd,phoff,SEEK_SET);
		sys_read(fd,&phdr,sizeof(phdr));
		if (phdr.p_type == PT_LOAD) {
			load_phdr(fd,&phdr);
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
	strcpy(cur->name,path);
	struct pt_regs* regs = (struct pt_regs*)((uint64_t)cur->self_kstack - sizeof(struct pt_regs));

	regs->csr_crmd = read_csr_crmd();
	prmd = read_csr_prmd() & ~(PLV_MASK);
	prmd |= PLV_USER;
	regs->csr_prmd = prmd;

	regs->regs[3]  = USER_STACK;
	regs->regs[22] = regs->regs[3];
	int entry = sys_exeload(path,NULL,NULL);
	regs->csr_era = (unsigned long)entry;

	printk("jump to proc...\n");
	asm volatile("addi.d $r3,%0,0;b user_ret;"::"g"((uint64_t)regs):"memory");
	return -1;
}

