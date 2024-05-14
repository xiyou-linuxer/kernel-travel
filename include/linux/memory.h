#ifndef __MEMORY_H
#define __MEMORY_H
#include <linux/types.h>
#include <bitmap.h>
#include <asm/bootinfo.h>
#include <asm/numa.h>
#include <linux/list.h>

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

#define MAX_PAGE_ORDER 10
#define MAX_ORDER_NR_PAGES (1 << MAX_PAGE_ORDER)

static inline int __ffs(unsigned int x)
{
    return __builtin_ffs(x);
}

#define FPI_NONE		(0)
#define FPI_TO_TAIL		(1)


#ifndef ARCH_PFN_OFFSET
#define ARCH_PFN_OFFSET		(0UL)
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

#define _struct_page_alignment	__aligned(2 * sizeof(unsigned long))

struct page {
//     // 存储 page 的定位信息以及相关标志位
	unsigned long flags;
	struct list_head buddy_list;

    union {
        struct {    /* Page cache and anonymous pages */
            // 用来指向物理页 page 被放置在了哪个 lru 链表上
            struct list_head lru;
            // 如果 page 为文件页的话，低位为0，指向 page 所在的 page cache
            // 如果 page 为匿名页的话，低位为1，指向其对应虚拟地址空间的匿名映射区 anon_vma
            struct address_space *mapping;
            // 如果 page 为文件页的话，index 为 page 在 page cache 中的索引
            // 如果 page 为匿名页的话，表示匿名页在对应进程虚拟内存区域 VMA 中的偏移
        //     pgoff_t index;
            // 在不同场景下，private 指向的场景信息不同
            unsigned long private;
        };
        
//         struct {    /* slab, slob and slub */
//             union {
//                 // 用于指定当前 page 位于 slab 中的哪个具体管理链表上。
//                 struct list_head slab_list;
//                 struct {
//                     // 当 page 位于 slab 结构中的某个管理链表上时，next 指针用于指向链表中的下一个 page
//                     struct page *next;
//                     // 表示 slab 中总共拥有的 page 个数
//                     int pages;  
//                     // 表示 slab 中拥有的特定类型的对象个数
//                     int pobjects;   
//                 };
//             };
//             // 用于指向当前 page 所属的 slab 管理结构
//             struct kmem_cache *slab_cache; 
        
//             // 指向 page 中的第一个未分配出去的空闲对象
//             void *freelist;     
//             union {
//                 // 指向 page 中的第一个对象
//                 void *s_mem;    
//                 struct {            /* SLUB */
//                     // 表示 slab 中已经被分配出去的对象个数
//                     unsigned inuse:16;
//                     // slab 中所有的对象个数
//                     unsigned objects:15;
//                     // 当前内存页 page 被 slab 放置在 CPU 本地缓存列表中，frozen = 1，否则 frozen = 0
//                     unsigned frozen:1;
//                 };
//             };
//         };
//         struct {    /* 复合页 compound page 相关*/
//             // 复合页的尾页指向首页
//             unsigned long compound_head;    
//             // 用于释放复合页的析构函数，保存在首页中
//             unsigned char compound_dtor;
//             // 该复合页有多少个 page 组成
//             unsigned char compound_order;
//             // 该复合页被多少个进程使用，内存页反向映射的概念，首页中保存
//             atomic_t compound_mapcount;
//         };

//         // 表示 slab 中需要释放回收的对象链表
//         struct rcu_head rcu_head;
    };

//     union {     /* This union is 4 bytes in size. */
//         // 表示该 page 映射了多少个进程的虚拟内存空间，一个 page 可以被多个进程映射
//         atomic_t _mapcount;

//     };

//     // 内核中引用该物理页的次数，表示该物理页的活跃程度。
//     atomic_t _refcount;

//     void *virtual;  // 内存页对应的虚拟内存地址

// } _struct_page_alignment;
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

#endif
