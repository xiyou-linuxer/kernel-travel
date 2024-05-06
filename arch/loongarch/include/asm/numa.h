#ifndef _ASM_NUMA_H_
#define _ASM_NUMA_H_

#include <linux/memory.h>
#include <linux/types.h>

// 目前是 UMA 架构，只有一个伪 NUMA 节点
#define MAX_NUMNODES 1
// 可分配的最大内存块的阶数（伙伴系统）
#define MAX_ORDER 10

enum migratetype {
	MIGRATE_UNMOVABLE,
	MIGRATE_MOVABLE,
	MIGRATE_RECLAIMABLE,
	MIGRATE_TYPES
};

enum zone_type {
	ZONE_DMA,
	ZONE_NORMAL,
	ZONE_MOVABLE,
	MAX_NR_ZONES
};

struct free_area {
	struct list_head	free_list[MIGRATE_TYPES];
	unsigned long		nr_free;
};


typedef struct pglist_data {
	struct zone node_zones[MAX_NR_ZONES];
	// struct zonelist node_zonelists[MAX_ZONELISTS];
	int nr_zones;

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

struct zone {
	long low_reserved_mem[MAX_NR_ZONES];
	int node;
	struct pglist_data	*zone_pgdat;
	unsigned long		zone_start_pfn;
	// atomic_long_t		managed_pages;
	unsigned long		spanned_pages;
	unsigned long		present_pages;
	const char		*name;
	// spinlock_t		lock;
	struct free_area	free_area[MAX_ORDER + 1];
};

void paging_init(void);

#endif