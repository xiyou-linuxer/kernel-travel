#include <linux/memory.h>
#include <bitmap.h>
#include <asm/loongarch.h>
#include <asm/page.h>
#include <asm/numa.h>
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
static unsigned long arch_zone_lowest_possible_pfn[MAX_NR_ZONES] __initdata;
static unsigned long arch_zone_highest_possible_pfn[MAX_NR_ZONES] __initdata;
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

static unsigned long __init zone_spanned_pages_in_node(int nid,unsigned long zone_type,
			unsigned long node_start_pfn,unsigned long node_end_pfn,
			unsigned long *zone_start_pfn,unsigned long *zone_end_pfn)
{
	unsigned long zone_low = arch_zone_lowest_possible_pfn[zone_type];
	unsigned long zone_high = arch_zone_highest_possible_pfn[zone_type];

	*zone_start_pfn = clamp(node_start_pfn, zone_low, zone_high);
	*zone_end_pfn = clamp(node_end_pfn, zone_low, zone_high);

	if (*zone_end_pfn < node_start_pfn || *zone_start_pfn > node_end_pfn)
		return 0;

	*zone_end_pfn = min(*zone_end_pfn, node_end_pfn);
	*zone_start_pfn = max(*zone_start_pfn, node_start_pfn);

	return *zone_end_pfn - *zone_start_pfn;
}

unsigned long __init zone_absent_pages_in_node(int nid,
			unsigned long range_start_pfn,
			unsigned long range_end_pfn)
{
	if(range_start_pfn == range_end_pfn)
		return 0;
	
	unsigned long nr_absent = range_end_pfn - range_start_pfn;
	unsigned long start_pfn, end_pfn;
	int i;

	for_each_mem_pfn_range(i, nid, &start_pfn, &end_pfn, NULL) {
		start_pfn = clamp(start_pfn, range_start_pfn, range_end_pfn);
		end_pfn = clamp(end_pfn, range_start_pfn, range_end_pfn);
		nr_absent -= end_pfn - start_pfn;
	}
	return nr_absent;
}

static void __init calculate_node_totalpages(struct pglist_data *pg_data,
			unsigned long node_start_pfn,unsigned long node_end_pfn)
{
	unsigned long realtotalpages = 0, totalpages = 0;

	for (enum zone_type i; i < MAX_NR_ZONES; ++i) {
		struct zone *zone = pg_data->node_zones + i;
		unsigned long zone_start_pfn, zone_end_pfn;
		unsigned long spanned, absent;
		unsigned long real_size;

		spanned = zone_spanned_pages_in_node(pg_data->node_id, i,
			node_start_pfn,node_end_pfn,&zone_start_pfn,&zone_end_pfn);

		absent = zone_absent_pages_in_node(pg_data->node_id,
			zone_start_pfn,zone_end_pfn);

		real_size = spanned - absent;

		if (spanned)
			zone->zone_start_pfn = zone_start_pfn;
		// 错误处理
		else
			zone->zone_start_pfn = 0;
		
		zone->spanned_pages = spanned;
		zone->present_pages = real_size;

		totalpages += spanned;
		realtotalpages += real_size;
	}

	pg_data->node_spanned_pages = totalpages;
	pg_data->node_present_pages = realtotalpages;
	printk("On node %d totalpages: %lu\n", pg_data->node_id, realtotalpages);
}

static void __init free_area_init_core(struct pglist_data *pgdat)
{
	return;
}

static void __init free_area_init_node(int nid)
{
	pg_data_t *pg_data = node_data[nid];
	unsigned long start_pfn = 0, end_pfn = 0;
	ASSERT(pg_data->nr_zones >= 0);
	get_pfn_range_for_nid(nid, &start_pfn, &end_pfn);

	pg_data->node_id = nid;
	pg_data->node_start_pfn = start_pfn;

	if(start_pfn != end_pfn) {
		printk("[node%d] : [mem %#018Lx-%#018Lx]\n", nid,
			(u64)start_pfn << PAGE_SHIFT,
			end_pfn ? ((u64)end_pfn << PAGE_SHIFT) - 1 : 0);
		calculate_node_totalpages(pg_data, start_pfn, end_pfn);
		
	} else {
		printk("[node%d] : MEMORYLESS\n", nid);
		return;
	}
	free_area_init_core(pg_data);



}

static void __init memmap_init(void)
{

}

void __init free_area_init(unsigned long *max_zone_pfn)
{
	unsigned long start_pfn = PHYS_PFN(memblock_start_of_DRAM());
	int nid;
	unsigned long end_pfn;
	memset(arch_zone_lowest_possible_pfn, 0,sizeof(arch_zone_lowest_possible_pfn));
	memset(arch_zone_highest_possible_pfn, 0,sizeof(arch_zone_highest_possible_pfn));

	for(int i = 0; i < MAX_NR_ZONES; ++i) {
		int zone = i;
		if(zone == ZONE_MOVABLE)
			continue;
		// 确保范围的结束PFN值不小于任何一个给定的PFN值。
		end_pfn = max(max_zone_pfn[zone],start_pfn);
		arch_zone_lowest_possible_pfn[zone] = start_pfn;
		arch_zone_highest_possible_pfn[zone] = end_pfn;
		start_pfn = end_pfn;
	}

	for (int i = 0; i < MAX_NR_ZONES; i++) {
		if (i == ZONE_MOVABLE)
			continue;
		printk("  %-8s ", zone_names[i]);
		if (arch_zone_lowest_possible_pfn[i] ==
				arch_zone_highest_possible_pfn[i])
			printk("empty\n");
		else
			printk("[mem %#018Lx-%#018Lx]\n",
				(u64)arch_zone_lowest_possible_pfn[i]
					<< PAGE_SHIFT,
				((u64)arch_zone_highest_possible_pfn[i]
					<< PAGE_SHIFT) - 1);
	}

		// NUMA 下要处理每个 node 这里只有一个 node 简化处理
		pg_data_t *pg_data = node_data[nid];
		free_area_init_node(nid);

	memmap_init();

}
