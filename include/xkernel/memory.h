#ifndef __MEMORY_H
#define __MEMORY_H
#include <xkernel/types.h>
#include <bitmap.h>
#include <asm/bootinfo.h>
#include <asm/numa.h>
#include <xkernel/list.h>
#include <asm-generic/int-ll64.h>
#include <xkernel/compiler.h>
#include <xkernel/rbtree.h>
#include <asm/page.h>

#define MAX_ADDRESS_SPACE_SIZE 0xffffffffffffffff

#define DIV_ROUND_UP(divd,divs) ((divd+divs-1)/divs)

#define PAGESIZE 4096
#define ENTRY_SIZE 8
#define PGD_IDX(addr) ((addr & 0x7fc0000000)>>30)
#define PMD_IDX(addr) ((addr & 0x003fe00000)>>21)
#define PTE_IDX(addr) ((addr & 0x00001ff000)>>12)
#define PAGE_OFFSET(addr) (addr & 0xfff)

#define PTE_V (1UL << 0)
#define PTE_D (1UL << 1)
#define PTE_PLV (3UL << 2)
#define PTE_G (1UL << 6)

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

static inline int mem_ffs(unsigned int x)
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
#ifndef page_to_pfn
#define page_to_pfn __page_to_pfn
#endif

#ifndef pfn_to_page 
#define pfn_to_page __pfn_to_page
#endif

#ifndef pfn_valid
static inline int pfn_valid(unsigned long pfn)
{
	extern unsigned long max_mapnr;
	unsigned long pfn_offset = ARCH_PFN_OFFSET;

	return pfn >= pfn_offset && (pfn - pfn_offset) < max_mapnr;
}
#endif

#define INVAILD_MMAP_CACHE ((struct vm_area_struct *)NULL)

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

#define PROT_READ	0x1		/* page can be read */
#define PROT_WRITE	0x2		/* page can be written */
#define PROT_EXEC	0x4		/* page can be executed */
#define PROT_NONE	0x0		/* page can not be accessed */

#define MAP_TYPE	0x0f		/* Mask for type of mapping */
#define MAP_FIXED	0x10		/* Interpret addr exactly */
#define MAP_ANONYMOUS	0x20		/* don't use a file */

#define VM_NONE		0x00000000

#define VM_READ		0x00000001	/* currently active flags */
#define VM_WRITE	0x00000002
#define VM_EXEC		0x00000004
#define VM_SHARED	0x00000008

// 虚拟内存区域描述符
struct vm_area_struct {
	struct mm_struct * vm_mm;
	unsigned long vm_start;
	unsigned long vm_end;
	// vma 在 mm_struct->mmap 双向链表中的前驱节点和后继节点
	struct vm_area_struct *vm_next, *vm_prev;
	// vma 在 mm_struct->mm_rb 红黑树中的节点
	struct rb_node vm_rb;
	// pgprot_t vm_page_prot;
	unsigned long vm_flags; 	/*指定内存映射方式*/
	struct file * vm_file;		/* File we map to (can be NULL). */
	unsigned long vm_pgoff;		/* Offset (within vm_file) in PAGE_SIZE */
	// void * vm_opts;
};

// 进程虚拟内存空间描述符
struct mm_struct {
	// 串联组织进程空间中所有的 VMA  的双向链表 
	unsigned long mmap_base;
	struct vm_area_struct *mmap;  /* list of VMAs */
	// 管理进程空间中所有 VMA 的红黑树
	struct rb_root mm_rb;
	struct vm_area_struct * mmap_cache;
	unsigned long free_area_cache;	/*记录上次成功分配的起始地址的缓存*/
	unsigned long total_vm;
	unsigned long start_code, end_code, start_data, end_data;
	unsigned long start_brk, brk, start_stack;
	unsigned long arg_start, arg_end, env_start, env_end;
	unsigned long map_count;
	unsigned long (*get_unmapped_area) (struct file *filp,
				unsigned long addr, unsigned long len,
				unsigned long pgoff, unsigned long flags);
	unsigned long rss;
	struct file * vm_file;		/* 映射的文件，匿名映射即为nullptr*/
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
void free_page(u64 vaddr);
void free_pages(u64 vstart,u64 count);
unsigned long get_kernel_pge(void);
void free_kernel_pge(void* k_page);
void page_table_add(uint64_t pd,uint64_t _vaddr,uint64_t _paddr,uint64_t attr);
u64 vaddr_to_paddr(u64 pd, u64 vaddr);
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
void mm_struct_init(struct mm_struct *mm);
#endif