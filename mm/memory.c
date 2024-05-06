#include <linux/memory.h>
#include <bitmap.h>
#include <asm/loongarch.h>
#include <asm/page.h>
#include <debug.h>
#include <linux/stdio.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/thread.h>
#include <linux/memblock.h>
#include <linux/init.h>
#include <linux/math.h>


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

//pgd:0x9000000008010000 -> 0x9000000090003000
//pmd:0x9000000090003200 -> 0x9000000090006000
//pt: 0x9000000090006080  -> 0x90005000
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

void malloc_usrpage(u64 pd,u64 vaddr)
{
	unsigned long paddr = get_page();
	page_table_add(pd,vaddr,paddr,PTE_V | PTE_PLV | PTE_D);
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

void free_kernel_pge(void* k_page)
{
	u64 bit_off = (((unsigned long)k_page & ~CSR_DMW1_BASE) >> 12) - 0x6;
	bitmap_set(&reserve_phy_pool.btmp, bit_off, 1);
	memset((void *)k_page,0,(int)(PAGE_SIZE));
}

void * kmalloc(u64 size)
{
	struct task_struct * curr =  running_thread();
	struct mm_struct * mm = curr->mm;
	return (void *)get_kernel_pge();
	// if(__builtin_constant_p(size) && size) {
	// 	unsigned long index;
	// 	if(size > KMALLOC_MAX_CACHE_SIZE)
	// }
}

void __init get_pfn_range_for_nid(unsigned int nid,
			unsigned long *start_pfn, unsigned long *end_pfn)
{
	unsigned long this_start_pfn, this_end_pfn;
	int i;

	*start_pfn = -1UL;
	*end_pfn = 0;

	for_each_mem_pfn_range(i, nid, &this_start_pfn, &this_end_pfn, NULL) {
		*start_pfn = min(*start_pfn, this_start_pfn);
		*end_pfn = max(*end_pfn, this_end_pfn);
	}

	if (*start_pfn == -1UL)
		*start_pfn = 0;
}

void __init free_area_init(unsigned long *max_zone_pfn)
{
	
}
