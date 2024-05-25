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
    Elf_Off e_shoff;
    Elf_Word e_flags;
    Elf_Half e_ehsize;
    Elf_Half e_phentsize;
    Elf_Half e_phnum;
    Elf_Half e_shentsize;
    Elf_Half e_shnum;
    Elf_Half e_shstrndx;
} Elf_Ehdr;

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

int64_t load(const char *path);
int sys_exeload(const char *path);
int sys_execve(const char *path, char *const argv[], char *const envp[]);

#endif
