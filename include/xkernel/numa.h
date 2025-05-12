#ifndef _ASM_NUMA_H_
#define _ASM_NUMA_H_

#include <xkernel/memory.h>
#include <xkernel/types.h>
#include <xkernel/list.h>

// 目前是 UMA 架构，只有一个伪 NUMA 节点
#define MAX_NUMNODES 1
// 可分配的最大内存块的阶数（伙伴系统）
#define MAX_ORDER 10

enum migratetype {
	MIGRATE_UNMOVABLE,   // 不可移动的内存页，通常指内核数据或一些关键结构，不能被迁移到其他节点或区域。
	MIGRATE_MOVABLE,    // 可移动的内存页，通常指用户空间的内存页，可以在内存回收或迁移时移动到其他节点或区域。
	MIGRATE_RECLAIMABLE, // 可回收的内存页，指在低内存条件下可以被回收的内存页，但在正常情况下可以保留。
	MIGRATE_TYPES       // 枚举类型的数量，表示迁移类型的总数。这个值通常用于数组的大小等目的。
};

enum zone_type {
	ZONE_DMA,
	ZONE_NORMAL,
	ZONE_MOVABLE,
	MAX_NR_ZONES = 3
};

struct free_area {
	struct list_head	free_list[MIGRATE_TYPES];
	unsigned long		nr_free;
};

enum zone_watermarks {
	WMARK_MIN,	/*最低水位线*/
	WMARK_LOW,	/*较低水位线*/
	WMARK_HIGH,	/*较高水位线*/
	// WMARK_PROMO,	/*促进水位线*/
	NR_WMARK	
};

#define zone_idx(zone)		((zone) - (zone)->zone_pgdat->node_zones)

struct zone {
	unsigned long _watermark[NR_WMARK];
	unsigned long watermark_boost;
	long low_reserved_mem[MAX_NR_ZONES];
	int node;
	struct pglist_data	*zone_pgdat;
	unsigned long		zone_start_pfn;
	// atomic_long_t		managed_pages;
	unsigned long		managed_pages;	// for tmp use
	unsigned long		spanned_pages;
	unsigned long		present_pages;
	const char		*name;
	struct bitmap		*pageblock_flags;
	// spinlock_t		lock;
	struct free_area	free_area[MAX_ORDER + 1];
	bool			initialized;
};

typedef struct pglist_data {
	struct zone node_zones[MAX_NR_ZONES];
	// struct zonelist node_zonelists[MAX_ZONELISTS];
	int nr_zones;
	struct page *node_mem_map;
	unsigned long node_start_pfn;
	unsigned long node_present_pages;
	unsigned long node_spanned_pages;
	int node_id;
	struct task_struct *kswapd;
	int kswapd_order;
	enum zone_type kswapd_highest_zoneidx;

	int kswapd_failures;

	unsigned long		totalreserve_pages;
	unsigned long		flags;

} pg_data_t;

struct alloc_context {
	struct zonelist *zonelist;
	void *nodemask;
	struct zoneref *preferred_zoneref;
	int migratetype;
	enum zone_type highest_zoneidx;
	bool spread_dirty_pages;
};

#define ALLOC_WMARK_MIN		WMARK_MIN
#define ALLOC_WMARK_LOW		WMARK_LOW
#define ALLOC_WMARK_HIGH	WMARK_HIGH
#define ALLOC_WMARK_MASK	(ALLOC_NO_WATERMARKS-1)
#define ALLOC_NO_WATERMARKS	0x04 /* don't check watermarks at all */


#define wmark_pages(z, i) (z->_watermark[i] + z->watermark_boost)

// static unsigned long pgdat_end_pfn(struct pglist_data *pgdat)
// {
// 	return pgdat->node_start_pfn + pgdat->node_spanned_pages;
// }

static inline bool zone_is_initialized(struct zone *zone)
{
	return zone->initialized;
}

extern struct pglist_data node_data[MAX_NUMNODES];

extern char * const zone_names[MAX_NR_ZONES];
void paging_init(void);

#endif
