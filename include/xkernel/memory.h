#ifndef __MEMORY_H
#define __MEMORY_H
#include <xkernel/types.h>
#include <bitmap.h>
#include <asm/bootinfo.h>
#include <asm/numa.h>
#include <xkernel/list.h>
#include <asm-generic/int-ll64.h>

#define DIV_ROUND_UP(divd,divs) ((divd+divs-1)/divs)

#define PAGESIZE 4096
#define ENTRY_SIZE 8
#define PGD_IDX(addr) ((addr & 0x7fc0000000)>>30)
#define PMD_IDX(addr) ((addr & 0x003fe00000)>>21)
#define PTE_IDX(addr) ((addr & 0x00001ff000)>>12)

#define PTE_V (1UL << 0)
#define PTE_D (1UL << 1)
#define PTE_PLV (3UL << 2)

#define DMW_MASK  (0x9000000000000000)

#define for_each_migratetype_order(order, type) \
	for (order = 0; order <= MAX_ORDER; order++) \
		for (type = 0; type < MIGRATE_TYPES; type++)

#define for_pglist_data_each_zone(zone, node_data) \
	for (i = 0, zone = &node_data->node_zones[i];	\
		i < node_data->nr_zones;	\
		++i,zone = &node_data->node_zones[i])

#define MAX_PAGE_ORDER 10
#define MAX_ORDER_NR_PAGES (1 << MAX_PAGE_ORDER)

static inline int __ffs(unsigned int x)
{
    return __builtin_ffs(x);
}

#define FPI_NONE		(0)
#define FPI_TO_TAIL		(1)


#ifndef ARCH_PFN_OFFSET
#define ARCH_PFN_OFFSET		(0x9000000090000UL)
#endif

#define __pfn_to_page(pfn)	(mem_map + ((pfn) - ARCH_PFN_OFFSET))
#define __page_to_pfn(page)	((unsigned long)((page) - mem_map) + \
				 ARCH_PFN_OFFSET)

#define page_to_pfn __page_to_pfn
#define pfn_to_page __pfn_to_page

#ifndef pfn_valid
static inline int pfn_valid(unsigned long pfn)
{
	extern unsigned long max_mapnr;
	unsigned long pfn_offset = ARCH_PFN_OFFSET;

	return pfn >= pfn_offset && (pfn - pfn_offset) < max_mapnr;
}
#endif

struct virt_addr {
    u64 vaddr_start;
    struct bitmap btmp;
};

struct pool {
	u64 paddr_start;
	struct bitmap btmp;
};

#define _struct_page_alignment	__aligned(2*sizeof(unsigned long))

struct page {
	unsigned long flags;		/* Atomic flags, some possibly
					 * updated asynchronously */
	atomic_t _count;		/* Usage count, see below. */
	union {
		atomic_t _mapcount;	/* Count of ptes mapped in mms,
					 * to show when page is mapped
					 * & limit reverse map searches.
					 */
		unsigned int inuse;	/* SLUB: Nr of objects */
	};
	union {
	    struct {
		unsigned long private;		/* Mapping-private opaque data:
					 	 * usually used for buffer_heads
						 * if PagePrivate set; used for
						 * swp_entry_t if PageSwapCache;
						 * indicates order in the buddy
						 * system if PG_buddy is set.
						 */
		struct address_space *mapping;	/* If low bit clear, points to
						 * inode address_space, or NULL.
						 * If page mapped as anonymous
						 * memory, low bit is set, and
						 * it points to anon_vma object:
						 * see PAGE_MAPPING_ANON below.
						 */
	    };
	    struct kmem_cache *slab;	/* SLUB: Pointer to slab */
	    struct page *first_page;	/* Compound tail pages */
	};
	union {
		pgoff_t index;		/* Our offset within mapping. */
		void *freelist;		/* SLUB: freelist req. slab lock */
	};
	// struct list_head lru;		/* Pageout list, eg. active_list
	// 				 * protected by zone->lru_lock !
	// 				 */
	struct list_head buddy_list;
	char __padding[8]; 
};

struct zoneref {
	struct zone *zone;
	int zone_idx;
};

struct mm_struct {
	u64 pgd;
};

extern struct pool reserve_phy_pool;
extern struct pool phy_pool;
extern struct page *mem_map;


extern char * const zone_names[3];

static inline void invalidate(void)
{
	asm volatile("invtlb 0x0,$r0,$r0");
}

static inline int
page_zonenum(const struct page *page)
{
	return 1;
}


static inline unsigned long
__find_buddy_pfn(unsigned long page_pfn, unsigned int order)
{
	return page_pfn ^ (1 << order);
}

static inline unsigned int
buddy_order(struct page *page)
{
	return page->private;
}

static inline bool
page_is_buddy(struct page *page, struct page *buddy,
				 unsigned int order)
{
	if (buddy_order(buddy) != order)
		return false;
	/*这里只管理 Normal，忽略*/
	// if (page_zone_id(page) != page_zone_id(buddy))
	// 	return false;
	return true;
}

static inline struct page *
find_buddy_page_pfn(struct page *page,
			unsigned long pfn, unsigned int order, unsigned long *buddy_pfn)
{
	unsigned long __buddy_pfn = __find_buddy_pfn(pfn, order);
	struct page *buddy;

	buddy = page + (__buddy_pfn - pfn);
	if (buddy_pfn)
		*buddy_pfn = __buddy_pfn;

	if (page_is_buddy(page, buddy, order))
		return buddy;
	return NULL;
}

#define set_buddy_order(page, order) set_page_private(page, order)

static inline void
set_page_private(struct page *page, unsigned long private)
{
	page->private = private;
}

u64 *pgd_ptr(u64 pd,u64 vaddr);
u64 *pmd_ptr(u64 pd,u64 vaddr);
u64 *pte_ptr(u64 pd,u64 vaddr);
u64 get_page(void);
u64 get_pages(u64 count);
unsigned long get_kernel_pge(void);
void free_kernel_pge(void* k_page);
void page_table_add(uint64_t pd,uint64_t _vaddr,uint64_t _paddr,uint64_t attr);
void malloc_usrpage_withoutopmap(u64 pd,u64 vaddr);
void malloc_usrpage(u64 pd,u64 vaddr);

void * kmalloc(u64 size);
void get_pfn_range_for_nid(unsigned int nid,
			unsigned long *start_pfn, unsigned long *end_pfn);
void free_area_init(unsigned long *max_zone_pfn);
void memblock_init(void);
void __free_pages_core(struct page *page, unsigned int order);
void mem_init(void);
struct page *__alloc_pages(gfp_t gfp, unsigned int order, int preferred_nid);

void __free_pages_ok(struct page *page, unsigned int order,
			    bool fpi_flags);
#endif
