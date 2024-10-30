#include <xkernel/slab.h>
#include <xkernel/memory.h>
#include <xkernel/kernel.h>
#include <sync.h>

#define	CACHE_CACHE 0

#define REAPTIMEOUT_CPUC	(2*HZ)
#define REAPTIMEOUT_LIST3	(4*HZ)

/*
 * Do not go above this order unless 0 objects fit into the slab.
 */
#define BREAK_GFP_ORDER_HI  1//BREAK_GFP_ORDER_HI 表示一个较高的“order（阶数）”值，用于表示“slab”分配器的对象大小的上限
#define BREAK_GFP_ORDER_LO  0//BREAK_GFP_ORDER_LO 表示一个较低的“order”值，用于表示“slab”分配器的对象大小的下限
static int slab_break_gfp_order = BREAK_GFP_ORDER_LO;//slab_break_gfp_order 用于存储当前的“order”值，用于控制“slab”分配器的行为

static struct arraycache_init initarray_cache =
    { {0, BOOT_CPUCACHE_ENTRIES, 1, 0} };
static struct arraycache_init initarray_generic =
    { {0, BOOT_CPUCACHE_ENTRIES, 1, 0} };

//cache_cache 中的 
#define NUM_INIT_LISTS (3 * MAX_NUMNODES)
struct kmem_list3 initkmem_list3[NUM_INIT_LISTS];

static struct list_head cache_chain;//用来维护所有 kmem_cache 实例（缓存池）的链表

/* 用于为 kmem_cache 结构本身分配小块内存的结构*/
static struct kmem_cache cache_cache = {
	.batchcount = 1,
	.limit = BOOT_CPUCACHE_ENTRIES,
	.shared = 1,
	.buffer_size = sizeof(struct kmem_cache),
	.name = "kmem_cache",
};

struct cache_names {
	char *name;
	char *name_dma;
};

static struct cache_names cache_names[] = {
#define CACHE(x) { .name = "size-" #x, .name_dma = "size-" #x "(DMA)" },
#include <xkernel/kmalloc_sizes.h>
	{NULL,}
#undef CACHE
};

//内核中预留的kmem类，从64字节～
struct cache_sizes malloc_sizes[] = {
#define CACHE(x) { .cs_size = (x) },
#include <xkernel/kmalloc_sizes.h>
	CACHE(ULONG_MAX)
#undef CACHE
};

#define INDEX_AC index_of(sizeof(struct arraycache_init))
#define INDEX_L3 index_of(sizeof(struct kmem_list3))

static int use_alien_caches = 1;

u32 reciprocal_value(u32 k)
{
	u64 val = (1LL << 32) + (k - 1);
	do_div(val, k);
	return (u32)val;
}

static void kmem_list3_init(struct kmem_list3 *parent)
{
	//初始化三种不同类型的slab链表
	INIT_LIST_HEAD(&parent->slabs_full);//使用完
	INIT_LIST_HEAD(&parent->slabs_partial);//使用过
	INIT_LIST_HEAD(&parent->slabs_free);//空闲
	parent->shared = NULL;
	parent->alien = NULL;
	parent->colour_next = 0;
	lock_init(&parent->list_lock);
	parent->free_objects = 0;
	parent->free_touched = 0;
}

//初始化每个 NUMA 节点上与特定 kmem_cache 对象相关的 kmem_list3 结构体，并设置每个节点的回收时间
static void set_up_list3s(struct kmem_cache *cachep, int index)
{
	int node;
	
	/* 将 cachep 的每个节点列表指向初始化的 kmem_list3 数组中的相应元素 */
	cachep->nodelists[0] = &initkmem_list3[index + 0];

	/* 设置 next_reap 字段，计算下次回收的时间 */
	cachep->nodelists[0]->next_reap = 
		    REAPTIMEOUT_LIST3 +                    /* 回收超时常量 */
		    ((unsigned long)cachep) % REAPTIMEOUT_LIST3; /* 随机化偏移量以避免同步，防止回收时间在同一时刻发生 */
}

void kmem_cache_init(void)
{
	// 初始化过程中使用的一些变量
	size_t left_over;
	struct cache_sizes *sizes;
	struct cache_names *names;
	int i;
	int order;
	int node;
	//系统暂时只启动了一个处理器核
	use_alien_caches = 0;
	for (i = 0; i < NUM_INIT_LISTS; i++) {
		kmem_list3_init(&initkmem_list3[i]);
		if (i < MAX_NUMNODES)
			cache_cache.nodelists[i] = NULL;
	}
	set_up_list3s(&cache_cache, CACHE_CACHE);

	// 如果总的内存超过32MB，设置slab分配器使用的页数更大，以减少碎片化
	if (totalram_pages > (32 << 20) >> PAGE_SHIFT)
		slab_break_gfp_order = BREAK_GFP_ORDER_HI;

	// 第一步: 创建缓存，用于管理 kmem_cache 结构
	node = 0;  // 获取当前 NUMA 节点的 ID(只有一个节点)

	// 初始化 cache_cache，管理所有缓存描述符
	INIT_LIST_HEAD(&cache_chain);  // 初始化 cache_chain 列表
	list_add(&cache_cache.next, &cache_chain);  // 将 cache_cache 加入链表
	cache_cache.colour_off = cache_line_size();  // 设置 cache 的颜色偏移量
	cache_cache.array[0] = &initarray_cache.cache;  // 设置 per-CPU 缓存
	cache_cache.nodelists[node] = &initkmem_list3[CACHE_CACHE + node];  // 设置节点列表

	// 设置 cache_cache 的大小，确保它对齐缓存行
	cache_cache.buffer_size = offsetof(struct kmem_cache, nodelists) +
				 nr_node_ids * sizeof(struct kmem_list3 *);
	cache_cache.buffer_size = ALIGN_MASK(cache_cache.buffer_size,
					cache_line_size());
	cache_cache.reciprocal_buffer_size =
		reciprocal_value(cache_cache.buffer_size);
// 估算缓存大小，确保有足够的空间来存放对象
	for (order = 0; order < MAX_ORDER; order++) {
		cache_estimate(order, cache_cache.buffer_size,
			cache_line_size(), 0, &left_over, &cache_cache.num);
		if (cache_cache.num)
			break;
	}
	BUG_ON(!cache_cache.num);  // 如果计算失败，触发 BUG
	cache_cache.gfporder = order;  // 设置缓存使用的页面顺序
	cache_cache.colour = left_over / cache_cache.colour_off;  // 设置缓存颜色
	cache_cache.slab_size = ALIGN_MASK(cache_cache.num * sizeof(kmem_bufctl_t) +
				      sizeof(struct slab), cache_line_size());  // 设置 slab 大小

	// 第二步: 创建 kmalloc 缓存，用于分配内存对象
	sizes = malloc_sizes;
	names = cache_names;

	// 创建内存池，确保能够正确分配 array cache 和 kmem_list3 结构
	sizes[INDEX_AC].cs_cachep = kmem_cache_create(names[INDEX_AC].name,
					sizes[INDEX_AC].cs_size,
					ARCH_KMALLOC_MINALIGN,
					ARCH_KMALLOC_FLAGS|SLAB_PANIC,
					NULL);

	// 如果 INDEX_AC 与 INDEX_L3 不同，也创建 L3 缓存
	if (INDEX_AC != INDEX_L3) {
		sizes[INDEX_L3].cs_cachep =
			kmem_cache_create(names[INDEX_L3].name,
				sizes[INDEX_L3].cs_size,
				ARCH_KMALLOC_MINALIGN,
				ARCH_KMALLOC_FLAGS|SLAB_PANIC,
				NULL);
	}

	slab_early_init = 0;  // slab 初始化标志

	// 为每种大小的 slab 缓存创建内存池
	while (sizes->cs_size != ULONG_MAX) {
		if (!sizes->cs_cachep) {
			sizes->cs_cachep = kmem_cache_create(names->name,
					sizes->cs_size,
					ARCH_KMALLOC_MINALIGN,
					ARCH_KMALLOC_FLAGS|SLAB_PANIC,
					NULL);
		}
		sizes++;
		names++;
	}

	// 第四步: 替换缓存的初始化数组
	{
		struct array_cache *ptr;

		ptr = kmalloc(sizeof(struct arraycache_init), GFP_NOWAIT);
		BUG_ON(cpu_cache_get(&cache_cache) != &initarray_cache.cache);
		memcpy(ptr, cpu_cache_get(&cache_cache),
		       sizeof(struct arraycache_init));
		spin_lock_init(&ptr->lock);
		cache_cache.array[smp_processor_id()] = ptr;

		ptr = kmalloc(sizeof(struct arraycache_init), GFP_NOWAIT);
		BUG_ON(cpu_cache_get(malloc_sizes[INDEX_AC].cs_cachep)
		       != &initarray_generic.cache);
		memcpy(ptr, cpu_cache_get(malloc_sizes[INDEX_AC].cs_cachep),
		       sizeof(struct arraycache_init));
		spin_lock_init(&ptr->lock);
		malloc_sizes[INDEX_AC].cs_cachep->array[smp_processor_id()] = ptr;
	}

	// 第五步: 替换 kmem_list3 的初始化数据
	{
		int nid;

		for_each_online_node(nid) {
			init_list(&cache_cache, &initkmem_list3[CACHE_CACHE + nid], nid);
			init_list(malloc_sizes[INDEX_AC].cs_cachep,
				  &initkmem_list3[SIZE_AC + nid], nid);

			if (INDEX_AC != INDEX_L3) {
				init_list(malloc_sizes[INDEX_L3].cs_cachep,
					  &initkmem_list3[SIZE_L3 + nid], nid);
			}
		}
	}

	// 标志 CPU 缓存已初始化
	g_cpucache_up = EARLY;
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