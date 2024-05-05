#include <exec.h>
#include <linux/memory.h>
#include <linux/thread.h>
#include <linux/stdio.h>
#include <stdint.h>
#include <linux/string.h>

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

static bool load_phdr(void *program,Elf_Phdr *phdr)
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

		page += PAGESIZE;
	}

	pro_seek(&program,phdr->p_offset);
	pro_read(program,(void*)phdr->p_vaddr,phdr->p_filesz);
	return true;
}

int64_t load(void *program)
{
	Elf_Ehdr ehdr;
	memset(&ehdr,0,sizeof(ehdr));
	pro_seek(&program,0);
	pro_read(program,&ehdr,sizeof(ehdr));

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
		pro_seek(&program,phoff);
		pro_read(program,&phdr,sizeof(phdr));
		if (phdr.p_type == PT_LOAD) {
			load_phdr(program,&phdr);
		}
		phoff += ehdr.e_phentsize;
	}

	ret = ehdr.e_entry;
done:
	return ret;
}


int sys_execv(void *program)
{
	program_begin = program;
	int64_t entry_point = load(program);
	if (entry_point == -1) {
		printk("sys_execv: load failed\n");
		return -1;
	}

	return entry_point;
}


