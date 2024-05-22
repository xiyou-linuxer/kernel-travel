#include <xkernel/memory.h>
#include <bitmap.h>
#include <asm/loongarch.h>
#include <asm/page.h>
#include <asm/numa.h>
#include <debug.h>
#include <xkernel/stdio.h>
#include <xkernel/types.h>
#include <xkernel/string.h>
#include <xkernel/sched.h>
#include <xkernel/thread.h>
#include <xkernel/memblock.h>
#include <xkernel/init.h>
#include <xkernel/math.h>
#include <xkernel/compiler.h>
#include "xkernel/mmap.h"

struct pool reserve_phy_pool __initdata;
struct pool phy_pool __initdata;
static unsigned long arch_zone_lowest_possible_pfn[MAX_NR_ZONES] __initdata;
static unsigned long arch_zone_highest_possible_pfn[MAX_NR_ZONES] __initdata;
static unsigned long nr_kernel_pages __initdata;
static unsigned long nr_all_pages __initdata;
void *high_memory;
struct page *mem_map;

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

void free_page(u64 vaddr)
{
	u64 bit_off = ((vaddr|CSR_DMW1_BASE) - phy_pool.paddr_start) >> 12;
	ASSERT(bit_off < phy_pool.btmp.btmp_bytes_len);
	bitmap_set(&phy_pool.btmp,bit_off,0);
}

u64 get_pages(u64 count)
{
	unsigned long page;
	u64 bit_off = bit_scan(&phy_pool.btmp,count);
	if (bit_off == -1){
		return 0;
	}
	for (u64 i = 0,b = bit_off ; i < count ; i++,b++)
		bitmap_set(&phy_pool.btmp,b,1);

	page = (((bit_off << 12) + phy_pool.paddr_start) | CSR_DMW1_BASE);
	memset((void *)page,0,(int)(PAGE_SIZE)*count);
	return page;
}

void free_pages(u64 vstart,u64 count)
{
	u64 bit_off = (vstart&~CSR_DMW1_BASE - phy_pool.paddr_start) >> 12;
	for (u64 i = 0,b = bit_off ; i < count ; i++,b++)
		bitmap_set(&phy_pool.btmp,b,0);
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

void malloc_usrpage(u64 pd,u64 vaddr)
{
	unsigned long paddr = get_page();
	page_table_add(pd,vaddr,paddr,PTE_V | PTE_PLV | PTE_D);
	struct task_struct *cur = running_thread();
	uint64_t bit_idx = (vaddr - cur->usrprog_vaddr.vaddr_start)/PAGESIZE;
	bitmap_set(&cur->usrprog_vaddr.btmp,bit_idx,1);
}

void malloc_usrpage_withoutopmap(u64 pd,u64 vaddr)
{
	unsigned long paddr = get_page();
	page_table_add(pd,vaddr,paddr,PTE_V | PTE_PLV | PTE_D);
	//memset((void *)vaddr,0,(int)(PAGE_SIZE));
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

u64 get_kernel_pages(u64 count)
{
	unsigned long page;
	u64 bit_off = bit_scan(&reserve_phy_pool.btmp,count);
	if (bit_off == -1){
		return 0;
	}
	for (u64 i = 0,b = bit_off ; i < count ; i++,b++)
		bitmap_set(&reserve_phy_pool.btmp,b,1);

	page = (((bit_off << 12) + reserve_phy_pool.paddr_start) | CSR_DMW1_BASE);
	memset((void *)page,0,(int)(PAGE_SIZE)*count);
	return page;
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

struct page * __init memmap_init(unsigned long size)
{
	if(size == 0)
		return NULL;

	return (struct page * )get_kernel_pages(CEIL_DIV(size,PAGE_SIZE));
}

void __init alloc_node_mem_map(struct pglist_data *pg_data)
{
	unsigned long start, offset, size, end;
	struct page *map;

	if (!pg_data->node_spanned_pages)
		return;

	start = pg_data->node_start_pfn & ~(MAX_ORDER_NR_PAGES - 1);
	offset = pg_data->node_start_pfn - start;

	end = ALIGN(pgdat_end_pfn(pg_data), MAX_ORDER_NR_PAGES);
	size = (end - start) * sizeof(struct page);
	map = memmap_init(size);
	if (!map)
		PANIC();
	pg_data->node_mem_map = map + offset;

	if (pg_data == &node_data[0]) {
		mem_map = node_data[0].node_mem_map;
		if (page_to_pfn(mem_map) != pg_data->node_start_pfn)
			mem_map -= offset;
	}
}

static void __init calculate_zone_watermarks(struct zone * zone)
{
	zone->_watermark[0] = (zone->present_pages * 256) >> 10;
	zone->_watermark[1] = (zone->present_pages * 320) >> 10;
	zone->_watermark[2] = (zone->present_pages * 384) >> 10;
}

void calculate_node_totalpages(struct pglist_data *pg_data,
			unsigned long node_start_pfn,unsigned long node_end_pfn)
{
	unsigned long realtotalpages = 0, totalpages = 0;

	for (enum zone_type i = 0; i < MAX_NR_ZONES; ++i) {
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
		calculate_zone_watermarks(zone);
	}

	pg_data->node_spanned_pages = totalpages;
	pg_data->node_present_pages = realtotalpages;
	printk("On node %d totalpages: %lu\n", pg_data->node_id, realtotalpages);
}

static void __meminit pgdat_init_internals(struct pglist_data *pg_data)
{
	// 需要初始化 pgdat->kswapd_wait 。是 kswapd 线程等待的等待队列。 前提：调度的 waitqueue
	/*需要等待某些条件满足或等待某些资源的释放，这时 kswapd 就会将自己放置在 kswapd_wait 等待队列中进行等待。*/
	// init_waitqueue_head(&pgdat->kswapd_wait);
	// ...
}

static void __meminit zone_init_internals(struct zone *zone, enum zone_type idx, int nid,
							unsigned long remaining_pages)
{
	zone->managed_pages = remaining_pages;
	zone->node = nid;
	zone->name = zone_names[idx];
	zone->zone_pgdat = &node_data[nid];
	zone->initialized = true;
	zone->watermark_boost = 0;
	// spin_lock_init(&zone->lock);
	// zone_seqlock_init(zone);
}

static void __meminit zone_init_free_lists(struct zone *zone)
{
	unsigned int order, t;
	for_each_migratetype_order(order, t) {
		INIT_LIST_HEAD(&zone->free_area[order].free_list[t]);
		zone->free_area[order].nr_free = 0;
	}
}


void __meminit init_currently_empty_zone(struct zone *zone,
					unsigned long zone_start_pfn,
					unsigned long size)
{
	struct pglist_data *pg_data = zone->zone_pgdat;
	int zone_idx = zone_idx(zone) + 1;
	// 确保 pgdat->nr_zones 记录了节点中所有内存区域的数量
	if (zone_idx > pg_data->nr_zones)
		pg_data->nr_zones = zone_idx;

	zone->zone_start_pfn = zone_start_pfn;
	zone_init_free_lists(zone);
}

static unsigned long __init calc_memmap_pages_size(unsigned long spanned_pages)
{
	return PAGE_ALIGN(spanned_pages * sizeof(struct page)) >> PAGE_SHIFT;
}

static void __init free_area_init_core(struct pglist_data *pg_data)
{
	int nid = pg_data->node_id;
	pgdat_init_internals(pg_data);

	for (enum zone_type j = 0; j < MAX_NR_ZONES; j++) {
		struct zone *zone = pg_data->node_zones + j;
		unsigned long size = zone->spanned_pages;
		unsigned long present_size = zone->present_pages;
		unsigned long memmap_pages_size = calc_memmap_pages_size(size);
		
		if(present_size > memmap_pages_size) {
			present_size -= memmap_pages_size;
			if(memmap_pages_size)
				printk("[%s zone]: %lu pages used for memmap\n",
					 zone_names[j], memmap_pages_size);
			else
				printk("[%s zone]: %lu memmap pages exceeds freesize %lu\n",
					zone_names[j], memmap_pages_size, present_size);
		}
		// 这里需要 DMA 的话还要减去为 DMA 保留的页
		// ...
		nr_kernel_pages += present_size;
		nr_all_pages += present_size;
		zone_init_internals(zone, j, nid, present_size);
		
		if (!size)
			continue;

		init_currently_empty_zone(zone, zone->zone_start_pfn, size);
	}
	return;
}

static void __init free_area_init_node(int nid)
{
	pg_data_t *pg_data = &node_data[nid];
	unsigned long start_pfn = 0, end_pfn = 0;
	ASSERT(pg_data->nr_zones >= 0);
	get_pfn_range_for_nid(nid, &start_pfn, &end_pfn);

	pg_data->node_id = nid;
	pg_data->node_start_pfn = start_pfn;

	if(start_pfn != end_pfn) {
		printk("[node%d] : [mem %lu-%lu]\n", nid,
			(u64)start_pfn << PAGE_SHIFT,
			end_pfn ? ((u64)end_pfn << PAGE_SHIFT) - 1 : 0);
		calculate_node_totalpages(pg_data, start_pfn, end_pfn);
		
	} else {
		printk("[node%d] : MEMORYLESS\n", nid);
		return;
	}
	alloc_node_mem_map(pg_data);
	free_area_init_core(pg_data);
}

void __init  free_area_init(unsigned long *max_zone_pfn)
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
			printk("[mem %lu-%lu]\n",
				(u64)arch_zone_lowest_possible_pfn[i]
					<< PAGE_SHIFT,
				((u64)arch_zone_highest_possible_pfn[i]
					<< PAGE_SHIFT) - 1);
	}

	free_area_init_node(nid);

}

static inline void del_page_from_free_list(struct page *page, struct zone *zone,
					   unsigned int order)
{
	list_del(&page->buddy_list);
	set_page_private(page, 0);
	zone->free_area[order].nr_free--;
}

static inline void add_to_free_list_tail(struct page *page, struct zone *zone,
					 unsigned int order, int migratetype)
{
	struct free_area *area = &zone->free_area[order];

	list_add_tail(&page->buddy_list, &area->free_list[migratetype]);
	area->nr_free++;
}

static inline void add_to_free_list(struct page *page, struct zone *zone,
				    unsigned int order, int migratetype)
{
	struct free_area *area = &zone->free_area[order];

	list_add(&page->buddy_list, &area->free_list[migratetype]);
	area->nr_free++;
}

/*判断给定的页面（page）和其伙伴页面（通过参数 buddy_pfn 指定）在指定阶数（order）上是否可以进行合并。*/
static inline bool
buddy_merge_likely(unsigned long pfn, unsigned long buddy_pfn,
		   struct page *page, unsigned int order)
{
	unsigned long higher_page_pfn;
	struct page *higher_page;

	if (order >= MAX_PAGE_ORDER - 1)
		return false;

	higher_page_pfn = buddy_pfn & pfn;
	higher_page = page + (higher_page_pfn - pfn);

	return find_buddy_page_pfn(higher_page, higher_page_pfn, order + 1,
			NULL) != NULL;
}

void free_one_page(struct zone *zone,
				struct page *page, unsigned long pfn,
				unsigned int order,
				int migratetype, bool fpi_flags)
{
	struct page *buddy;
	unsigned long buddy_pfn = 0;
	unsigned long combined_pfn;
	bool to_tail;
	ASSERT(zone_is_initialized(zone) == true);
	printk("%p\n",zone);
	printk("%p\n",zone);
	printk("%lu\n",order);
	printk("%lu\n",pfn);
	while (order < MAX_PAGE_ORDER) {
		// printk("find_buddy_page_pfn function done\n");
		buddy = find_buddy_page_pfn(page, pfn, order, &buddy_pfn);
		if (!buddy)
			// 不需要合并
			goto done_merging;
		
		del_page_from_free_list(buddy, zone, order);
		// printk("del page for list done \n");
		combined_pfn = buddy_pfn & pfn;
		page = page + (combined_pfn - pfn);
		pfn = combined_pfn;
		order++;
	}
done_merging:
	set_buddy_order(page, order);
	if(fpi_flags == FPI_TO_TAIL)
		to_tail = true;
	else
	 	to_tail = buddy_merge_likely(pfn, buddy_pfn, page, order);
	
	if (to_tail)
		add_to_free_list_tail(page, zone, order, migratetype);
	else
		add_to_free_list(page, zone, order, migratetype);
}


static struct zone *
page_zone(const struct page *page)
{
	return (struct zone *)&node_data[0].node_zones[page_zonenum(page)];
}

// static
void __free_pages_ok(struct page *page, unsigned int order,
			    bool fpi_flags)
{
	int migratetype;
	unsigned long pfn = page_to_pfn(page);
	struct zone *zone = page_zone(page);
	free_one_page(zone, page, pfn, order, MIGRATE_UNMOVABLE, fpi_flags);
}

void __init __free_pages_core(struct page *page,unsigned int order)
{
	/*清除 struct page 中的数据*/
	// ...
	// printk("pageaddr:0x%llx\n",page);
	__free_pages_ok(page,order,FPI_TO_TAIL);   
}

// static inline
struct page *get_page_from_free_area(struct free_area *area,
					    int migratetype)
{
	return list_first_entry_or_null(&area->free_list[migratetype],
					struct page, buddy_list);
}

// static inline
void expand(struct zone *zone, struct page *page,
	int low, int high, int migratetype)
{
	unsigned long size = 1 << high;

	while (high > low) {
		high--;
		size >>= 1;
		// if (set_page_guard(zone, &page[size], high, migratetype))
		// 	continue;
		// 更新到新的 free_list 中
		add_to_free_list(&page[size], zone, high, migratetype);
		// 设置 page 的 private 为 high 的 order
		set_buddy_order(&page[size], high);
	}
}


// static __always_inline
struct page *__rmqueue_smallest(struct zone *zone, unsigned int order,
				int migratetype)
{
	unsigned int current_order;
	struct free_area *area;
	struct page *page;

	/* 从当前分配阶 order 开始在伙伴系统对应的  free_area[order]  里查找合适尺寸的内存块*/
	for (current_order = order; current_order <= MAX_ORDER; ++current_order) {
		area = &(zone->free_area[current_order]);
		page = get_page_from_free_area(area, migratetype);
		if (!page)
			// 没有空闲页，继续向上一级寻找
			continue;
		// 从当前的 free_list 的链表中删除
		del_page_from_free_list(page, zone,current_order);
		// 更新到更低的 free_list 中
		expand(zone, page, order, current_order, migratetype);
		// 设置页面的迁移类型
		page->index = migratetype;
		return page;
	}
	// 内存分配失败返回 null
	return NULL;
}


// static __always_inline
struct page *__rmqueue(struct zone *zone, unsigned int order, int migratetype,
                        unsigned int alloc_flags)
{
	struct page *page;

retry:
	page = __rmqueue_smallest(zone, order, migratetype);
	// 如果分配失败
	if(unlikely(!page)) {
		/*CMA fallback*/
		goto retry;
	}
	return page;
}

struct page *rmqueue(struct zone *preferred_zone,
		     struct zone *zone, unsigned int order,
		     gfp_t gfp_flags, unsigned int alloc_flags,
		     int migratetype)
{
	struct page *page;
	/*分配单个物理页*/
	if (likely(order == 0)) {
		/*先从 CPU 高速缓存列表 pcplist 中直接获取*/
		// goto out;
	}
	/*加锁，关中断*/
	do {
		page = __rmqueue(zone, order, migratetype, alloc_flags);
	} while (!page);
	/*解锁*/
	if(unlikely(!page)) {
		goto failed;
	}
	/*更新 zone 信息*/
out:
	return page;
failed:
	return NULL;
}

static inline bool zone_watermark_fast(struct zone *z, unsigned int order,
				unsigned long mark, int highest_zoneidx,
				unsigned int alloc_flags, gfp_t gfp_mask)
{
	return true;
}

struct page *
get_page_from_freelist(gfp_t gfp_mask, unsigned int order, int alloc_flags,
						const struct alloc_context *ac)
{
	struct zone * zone;
	int i;
retry:
	for_pglist_data_each_zone(zone, node_data) {
	if (zone->name == zone_names[1]) {
		struct page *page;
		unsigned long mark;
		mark = wmark_pages(zone, alloc_flags & ALLOC_WMARK_MASK);
		if (!zone_watermark_fast(zone, order, mark,
				       ac->highest_zoneidx, alloc_flags,
				       gfp_mask)) {
			// 如果不能快速分配，直接返回 NULL
			return NULL;
		}
try_this_node:
		// 尝试从伙伴系统获取
		// page = rmqueue(ac->preferred_zoneref->zone, zone, order,
		// 		gfp_mask, alloc_flags, ac->migratetype);
		page = rmqueue(NULL, zone, order,
				gfp_mask, alloc_flags, MIGRATE_UNMOVABLE);
		if(page) {
			/*初始化当前 struct page*/
			return page;
		} else {
			/*分配失败重试*/
			goto retry;
		}
	}
	}
	return NULL;
}


struct page *__alloc_pages(gfp_t gfp, unsigned int order, int preferred_nid)
{
	struct page *page;
	unsigned int alloc_flags = ALLOC_WMARK_LOW;
	gfp_t alloc_gfp; /* The gfp_t that was actually used for allocation */
	struct alloc_context ac = { };
	
	if (order > MAX_PAGE_ORDER)
		return NULL;
	/* gfp 逻辑处理*/
	
	/*预先 alloc page*/
	// if (!prepare_alloc_pages(gfp, order, preferred_nid, &alloc_gfp))
	// 	return NULL;

	/*第一次尝试分配*/
	page = get_page_from_freelist(alloc_gfp, order, alloc_flags, &ac);
	if (likely(page))
		goto out;
	/*slow path alloc*/
out:
	return page;
}

void __init mem_init(void)
{
	high_memory = (void *) __va(max_low_pfn << PAGE_SHIFT);
	memblock_free_all();
}

void mm_struct_init(struct mm_struct *mm)
{
	mm->mmap = NULL;
	mm->mm_rb = RB_ROOT;
	mm->mmap_cache = INVAILD_MMAP_CACHE;
	mm->free_area_cache = 0;
	mm->map_count = 0;
	mm->rss = 0;
	mm->vm_file = NULL;
	mm->start_code = mm->end_code = 0;
	mm->start_data = mm->end_data = 0;
	mm->arg_start = mm->arg_end = 0;
	mm->env_start = mm->env_start = 0;
	mm->start_brk = mm->brk = mm->start_stack = 0;
	
	/* 后面可以给不同架构做适配*/
	mm->get_unmapped_area = get_unmapped_area;
	
	return;
}