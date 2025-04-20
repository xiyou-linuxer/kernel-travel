#ifndef __ASM_NUMA_H
#define __ASM_NUMA_H

enum migratetype {
	MIGRATE_UNMOVABLE,   // 不可移动的内存页，通常指内核数据或一些关键结构，不能被迁移到其他节点或区域。
	MIGRATE_MOVABLE,    // 可移动的内存页，通常指用户空间的内存页，可以在内存回收或迁移时移动到其他节点或区域。
	MIGRATE_RECLAIMABLE, // 可回收的内存页，指在低内存条件下可以被回收的内存页，但在正常情况下可以保留。
	MIGRATE_TYPES       // 枚举类型的数量，表示迁移类型的总数。这个值通常用于数组的大小等目的。
};

enum zone_watermarks {
	WMARK_MIN,	/*最低水位线*/
	WMARK_LOW,	/*较低水位线*/
	WMARK_HIGH,	/*较高水位线*/
	// WMARK_PROMO,	/*促进水位线*/
	NR_WMARK	
};

#define ALLOC_WMARK_MIN		WMARK_MIN
#define ALLOC_WMARK_LOW		WMARK_LOW
#define ALLOC_WMARK_HIGH	WMARK_HIGH
#define ALLOC_WMARK_MASK	(ALLOC_NO_WATERMARKS-1)
#define ALLOC_NO_WATERMARKS	0x04 /* don't check watermarks at all */

enum zone_type {
	ZONE_DMA,
	ZONE_NORMAL,
	ZONE_MOVABLE,
	MAX_NR_ZONES = 3
};



#endif /* __ASM_NUMA_H */
