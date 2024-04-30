#include <linux/memory.h>
#include <bitmap.h>
#include <asm/loongarch.h>
#include <asm/page.h>
#include <debug.h>
#include <linux/stdio.h>
#include <linux/types.h>
#include <linux/string.h>

#define PAGESIZE 4096

struct pool reserve_phy_pool;
struct pool phy_pool;

uint64_t get_page(void)
{
	unsigned long page;
	u64 bit_off = bit_scan(&phy_pool.btmp,1);
	if (bit_off == -1){
		return 0;
	}
	bitmap_set(&phy_pool.btmp,bit_off,1);
	page = (((bit_off << 12) + phy_pool.paddr_start) | CSR_DMW1_BASE);
	memset((void *)page,0,(int)(PAGE_SIZE));
	return page;
}

u64 *pgd_ptr(u64 pd,u64 vaddr)
{
	return (u64*)(pd + PGD_IDX(vaddr) * ENTRY_SIZE);
}

u64 *pmd_ptr(u64 pd,u64 vaddr)
{
	u64 pmd;
	u64 *pgd = pgd_ptr(pd,vaddr);
	if(*pgd)
		pmd = *pgd | CSR_DMW1_BASE;
	else {
		pmd = get_page();
		*pgd = pmd;
	}
	return (u64*)(pmd + PMD_IDX(vaddr) * ENTRY_SIZE);
}

u64 *pte_ptr(u64 pd,u64 vaddr)
{
	u64 pt;
	u64 *pmd = pmd_ptr(pd,vaddr);
	if (*pmd)
		pt = *pmd | CSR_DMW1_BASE;
	else {
		pt = get_page();
		*pmd = pt;
	}
	return (u64*)(pt + PTE_IDX(vaddr) * ENTRY_SIZE);
}


void page_table_add(u64 pd,u64 _vaddr,u64 _paddr,u64 attr)
{
    u64 *pte = pte_ptr(pd,_vaddr);
    if (*pte) {
        printk("page_table_add: try to remap!!!\n");
        BUG();
    }
    *pte = _paddr | attr;
    // 刷新 TLB
    invalidate();
}

unsigned long get_kernel_pge(void)
{
	unsigned long k_page;
	u64 bit_off = bit_scan(&reserve_phy_pool.btmp,1);
	if(bit_off == -1) {
		return 0;
	}
	bitmap_set(&reserve_phy_pool.btmp, bit_off, 1);
	k_page = ((bit_off + 0x6) << 12) | CSR_DMW1_BASE;
	memset((void *)k_page,0,(int)(PAGE_SIZE));
	return k_page;
}

