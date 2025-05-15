#ifndef _ASM_BOOTINFO_H
#define _ASM_BOOTINFO_H

#include <asm/thread_info.h>

#define KERNEL_STACK_SIZE	0x00001000   // 4K

#ifndef __ASSEMBLY__

#include <xkernel/types.h>
#include <xkernel/init.h>
#include <asm/boot_param.h>


extern char arcs_cmdline[512];

extern void early_init(void);

extern void __init fw_init_environ(void);

extern char _start[];
extern char _end[];

struct bpi_mem_banks_t {
	phys_addr_t bank_data[16 * 2];
	uint32_t bank_nr;
};

extern struct bpi_mem_banks_t bpi_mem_banks;

void init_environ(void);
void memblock_init(void);

extern union thread_union init_thread_union;


#endif /* !__ASSEMBLY__ */
#endif /* _ASM_BOOTINFO_H */
