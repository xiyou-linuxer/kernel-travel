#ifndef __EXEC_H
#define __EXEC_H
#include <stdint.h>
#include <xkernel/thread.h>

#define NIDENT	16

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6

typedef uint32_t Elf_Word;
typedef uint64_t Elf_XWord;
typedef uint64_t Elf_Addr;
typedef uint64_t Elf_Off;
typedef uint16_t Elf_Half;

typedef struct
{
    unsigned char e_ident[NIDENT];
    Elf_Half e_type;
    Elf_Half e_machine;
    Elf_Word e_version;
    Elf_Addr e_entry;
    Elf_Off e_phoff;
    Elf_Off e_shoff;		/*节头表（Section Header Table）在文件中的偏移量*/
    Elf_Word e_flags;
    Elf_Half e_ehsize;
    Elf_Half e_phentsize;
    Elf_Half e_phnum;
    Elf_Half e_shentsize;	/*节头表中每个条目的大小（字节）*/
    Elf_Half e_shnum;		/*节头表中的条目数*/
    Elf_Half e_shstrndx;	/*节头表字符串表*/
} Elf_Ehdr;

/*ELF 64 位适用*/
typedef struct
{
	Elf_Word p_type;
	Elf_Word  p_flags;
	Elf_Off  p_offset;
	Elf_Addr p_vaddr;
	Elf_Addr p_paddr;
	Elf_XWord p_filesz;
	Elf_XWord p_memsz;
	Elf_XWord p_align;
} Elf_Phdr;

#define PF_R		0x4
#define PF_W		0x2
#define PF_X		0x1

#define ELF_MIN_ALIGN	PAGE_SIZE
#define ELF_CORE_EFLAGS	0
#define ELF_PAGESTART(_v) ((_v) & ~(unsigned long)(ELF_MIN_ALIGN-1))
#define ELF_PAGEOFFSET(_v) ((_v) & (ELF_MIN_ALIGN-1))
#define ELF_PAGEALIGN(_v) (((_v) + ELF_MIN_ALIGN - 1) & ~(ELF_MIN_ALIGN - 1))

int64_t load(const char *path);
int sys_exeload(const char *path);
int sys_execve(const char *path, char *const argv[], char *const envp[]);

#endif
