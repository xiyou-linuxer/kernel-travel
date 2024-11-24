#ifndef _LINUX_SLAB_DEF_H
#define	_LINUX_SLAB_DEF_H

#include <asm/page.h>		/* kmalloc_sizes.h needs PAGE_SIZE */
#include <asm/cache.h>		/* kmalloc_sizes.h needs L1_CACHE_BYTES */
#include <xkernel/compiler.h>
#include <xkernel/list.h>
#include <sync.h>
#include <xkernel/thread.h>

/*
 * 在分配slab块时传递给函数的参数
 */
#define SLAB_DEBUG_FREE		0x00000100UL	/* DEBUG: Perform (expensive) checks on free */
// 导致slab层在已分配的内存周围插入“红色警戒区”以探测缓冲越界
#define SLAB_RED_ZONE		0x00000400UL	/* DEBUG: Red zone objs in a cache */
// 使slab层用已知的值（a5a5a5a5）填充slab。这就是所谓的中毒，有利于对未初始化内存的访问
#define SLAB_POISON		0x00000800UL	/* DEBUG: Poison objects */
// 所有对象按高速缓存行对齐。
#define SLAB_HWCACHE_ALIGN	0x00002000UL	/* Align objs on cache lines */
// slab层使用可以执行DMA的内存给每个slab分配空间。
#define SLAB_CACHE_DMA		0x00004000UL	/* Use GFP_DMA memory */
#define SLAB_STORE_USER		0x00010000UL	/* DEBUG: Store the last owner for bug hunting */
// 当分配失败时提醒slab层
#define SLAB_PANIC		0x00040000UL	/* Panic if kmem_cache_create() fails */

//该宏定义指定了在系统引导时，每个 CPU 缓存中的初始缓存条目数
#define BOOT_CPUCACHE_ENTRIES	1

struct array_cache {
	unsigned int avail;
	unsigned int limit;
	unsigned int batchcount;
	unsigned int touched;
    struct lock lock;
	//spinlock_t lock;
	void *entry[];	/*
			 * Must have this definition in here for the proper
			 * alignment of array_cache. Also simplifies accessing
			 * the entries.
			 */
};

//用于在初始化阶段关联 array_cache 与 cp 
struct arraycache_init {
	struct array_cache cache;
	void *entries[BOOT_CPUCACHE_ENTRIES];
};


// 包含三个slab链表，slabs_full，slabs_partial和slabs_empty
struct kmem_list3 {
	struct list_head slabs_partial;	/* partial list first, better asm code */
	struct list_head slabs_full;
	struct list_head slabs_free;//未被分配的
	unsigned long free_objects;
	unsigned int free_limit;
	unsigned int colour_next;	/* Per-node cache coloring */
	struct lock list_lock;
	struct array_cache *shared;	/* shared per node */
	struct array_cache **alien;	/* on other nodes */
	unsigned long next_reap;	/* updated without locking */
	int free_touched;		/* updated without locking */
};

// kmem_cache 结构表示每个高速缓存（cache），它包含了管理 slab 分配器的元数据。
struct kmem_cache {
    /* 1) 每个CPU的缓存数据，在每次分配或释放时都会被访问 */
    struct array_cache *array[NR_CPUS]; 
    // 每个CPU对应的数组缓存，用于高速分配和释放对象。

    /* 2) Cache的可调参数，受 cache_chain_mutex 保护 */
    unsigned int batchcount;  // 每次从slab中分配/释放的对象批次大小。
    unsigned int limit;       // 每个CPU缓存中的最大对象数量。
    unsigned int shared;      // 每个CPU缓存间共享的对象数量限制。

    unsigned int buffer_size;            // 对象的大小，单位是字节。
    u32 reciprocal_buffer_size;          // 优化除法计算的值，跟 buffer_size 相关。

    /* 3) 在分配和释放时由后端频繁访问 */
    unsigned int flags;       // 常量标志位，定义了该缓存的特性，如是否调试、追踪等。
    unsigned int num;         // 每个 slab 中包含的对象数量。

    /* 4) cache_grow/shrink 调整缓存大小时使用的参数 */
    unsigned int gfporder;    // 每个 slab 占用的页数，以 2 的幂次方形式存储。

    // 强制指定的 GFP 标志（用于内存分配），如 GFP_DMA，控制分配区域等。
    gfp_t gfpflags;

    size_t colour;            // cache 的着色范围，用于减少 CPU 缓存冲突。
    unsigned int colour_off;  // 着色偏移量，保证不同对象的起始位置不同。
    struct kmem_cache *slabp_cache;  // 指向管理 slab 描述符的缓存。
    unsigned int slab_size;   // slab 的总大小，包括对象和管理数据。
    unsigned int dflags;      // 动态标志位，用于控制 slab 缓存的行为。

    // 对象的构造函数指针，分配对象时会调用，用于初始化对象。
    void (*ctor)(void *obj);

    /* 5) cache 的创建/移除信息 */
    const char *name;         // 缓存的名称，用于调试和识别。
    struct list_head next;    // 链表节点，用于将缓存链接到全局缓存链表中。

    /*
     * nodelists[] 位于 kmem_cache 结构的末尾，因为我们希望根据 nr_node_ids
     * 来动态调整这个数组的大小，而不是使用 MAX_NUMNODES（详见 kmem_cache_init()）。
     * 仍然使用 [MAX_NUMNODES] 是因为 cache_cache 是静态定义的，因此我们为最大数量的节点保留空间。
     */
    struct kmem_list3 *nodelists[MAX_NUMNODES]; // 每个节点的列表，管理缓存 slab 的分配和释放。

};

static enum {
	NONE,///尚未初始化，CPU 缓存系统没有启用。
	PARTIAL_AC,//处理器的 array 缓存已初始化，但更高级的缓存（如 kmem_list3）尚未设置。
	PARTIAL_L3,//部分初始化，kmem_list3 缓存已分配并初始化。
	EARLY,//早期初始化阶段，通常用于指代部分 CPU 缓存或数据结构已经准备好，但尚未完全启用。
	FULL//已经完全启用
} g_cpucache_up;

struct cache_sizes {
    size_t cs_size;                  // 描述缓存对象的大小，表示此缓存存储的每个对象的字节数。
    struct kmem_cache *cs_cachep;    // 指向内核缓存管理结构的指针，用于管理指定大小的缓存对象。
};
struct kmem_cache *kmem_cache_create (const char *name, size_t size, size_t align, unsigned long flags, void (*ctor)(void *));
void * kmalloc(u64 size,gfp_t flags);
#endif	/* _LINUX_SLAB_DEF_H */
