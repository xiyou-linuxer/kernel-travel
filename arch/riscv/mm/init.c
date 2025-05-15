#include <xkernel/init.h>
#include <xkernel/linkage.h>
#include <xkernel/types.h>
#include <asm/page.h>

#define PTRS_PER_PGD    (PAGE_SIZE / sizeof(pgd_t))

void *dtb_early_va;
static phys_addr_t dtb_early_pa __initdata;

pgd_t early_pg_dir[PTRS_PER_PGD] __initdata __aligned(PAGE_SIZE);

asmlinkage void __init setup_vm(uintptr_t dtb_pa)
{
	dtb_early_va = (void *)dtb_pa;
	dtb_early_pa = dtb_pa;
}