#ifndef __XKERNEL_GFP_H
#define __XKERNEL_GFP_H

#include <xkernel/memory.h>

struct vm_area_struct;

// 内存分配时的标志，分为三类，行为修饰符、区修饰符和类型。
// 行为修饰符表示内核应该如何分配内存，某些特定情况，只能使用特定方法分配内存，如中断中，分配内存不能睡眠
// 区修饰符表示从哪分配内存
// 类型标志组合了行为修饰符和区修饰符，便于使用。

/*
 * GFP bitmasks..
 *
 * Zone modifiers (see linux/mmzone.h - low three bits)
 *
 * Do not put any conditional on these. If necessary modify the definitions
 * without the underscores and use the consistently. The definitions here may
 * be used in bit comparisons.
 */
/*
 * GFP 位掩码..
 *
 * 区域修饰符（参见 linux/mmzone.h - 低三位）
 *
 * 不要对这些进行任何条件判断。如有必要，修改下划线开头的定义
 * 并一致地使用。这里的定义可能用于位比较。
 */

// 区域修饰符，不存在__GFP_NORMAL，因为如果不指定任何标志，默认是从ZONE_NORMAL（优先）和ZONE_DMA分配内存

// 强制只从ZONE_DMA分配内存
#define __GFP_DMA	((__force gfp_t)0x01u)
// 从ZONE_HIGHMEM（优先）或ZONE_NORMAL分配
#define __GFP_HIGHMEM	((__force gfp_t)0x02u)
// 只在ZONE_DMA32分配
#define __GFP_DMA32	((__force gfp_t)0x04u)
/* 页面是可移动的，意味着可以被页面迁移机制移动或回收 */
#define __GFP_MOVABLE	((__force gfp_t)0x08u)  /* Page is movable */
/* 区域掩码，包含所有前面定义的区域修饰符 */
#define GFP_ZONEMASK	(__GFP_DMA|__GFP_HIGHMEM|__GFP_DMA32|__GFP_MOVABLE)
/*
 * Action modifiers - doesn't change the zoning
 *
 * __GFP_REPEAT: Try hard to allocate the memory, but the allocation attempt
 * _might_ fail.  This depends upon the particular VM implementation.
 *
 * __GFP_NOFAIL: The VM implementation _must_ retry infinitely: the caller
 * cannot handle allocation failures.  This modifier is deprecated and no new
 * users should be added.
 *
 * __GFP_NORETRY: The VM implementation must not retry indefinitely.
 *
 * __GFP_MOVABLE: Flag that this page will be movable by the page migration
 * mechanism or reclaimed
 */
/*
 * 行为修饰符 - 不改变区域设置
 *
 * __GFP_REPEAT: 努力尝试分配内存，但分配尝试
 * _可能_会失败。这取决于特定的虚拟内存实现。
 *
 * __GFP_NOFAIL: 虚拟内存实现 _必须_ 无限重试：调用者
 * 无法处理分配失败。此修饰符已废弃，不应添加新用户。
 *
 * __GFP_NORETRY: 虚拟内存实现不得无限重试。
 *
 * __GFP_MOVABLE: 标记该页面将通过页面迁移机制移动或被回收
 */

// 行为修饰符

// 分配器可以睡眠
#define __GFP_WAIT	((__force gfp_t)0x10u)	/* Can wait and reschedule? */
// 分配器可以访问紧急事件缓冲池
/* 请求分配非常紧急的内存，注意与__GFP_HIGHMEM的区分，__GPF_HIGHMEM指从高端内存域分配内存 */
#define __GFP_HIGH	((__force gfp_t)0x20u)	/* Should access emergency pools? */
// 分配器可以启动磁盘IO
#define __GFP_IO	((__force gfp_t)0x40u)	/* Can start physical IO? */
// 分配器可以启动文件系统IO
#define __GFP_FS	((__force gfp_t)0x80u)	/* Can call down to low-level FS? */
// 分配器应该使用高速缓存中快要淘汰出去的页
#define __GFP_COLD	((__force gfp_t)0x100u)	/* Cache-cold page required */
// 分配器将不打印失败警告
#define __GFP_NOWARN	((__force gfp_t)0x200u)	/* Suppress page allocation failure warning */
// 分配器在分配失败时重复进行分配，但是这次分配还存在失败可能
#define __GFP_REPEAT	((__force gfp_t)0x400u)	/* See above */
// 分配器将无限期地重复进行分配。分配不能失败
#define __GFP_NOFAIL	((__force gfp_t)0x800u)	/* See above */
// 分配器在分配失败时绝对不会重新分配
#define __GFP_NORETRY	((__force gfp_t)0x1000u)/* See above */
// 添加混合页元数据，在hugetlb的代码内部使用
#define __GFP_COMP	((__force gfp_t)0x4000u)/* Add compound page metadata */
/* 成功返回时返回零页 */
#define __GFP_ZERO	((__force gfp_t)0x8000u)/* Return zeroed page on success */
/* 不使用紧急储备 */
#define __GFP_NOMEMALLOC ((__force gfp_t)0x10000u) /* Don't use emergency reserves */
/* 只能在当前进程可运行的cpu关联的内存节点上分配内存，如果进程可在所有cpu上运行，该标志无意义 */
#define __GFP_HARDWALL   ((__force gfp_t)0x20000u) /* Enforce hardwall cpuset memory allocs */
/* 无后备策略，仅此节点 */
#define __GFP_THISNODE	((__force gfp_t)0x40000u)/* No fallback, no policies */
/* 页面是可回收的 */
#define __GFP_RECLAIMABLE ((__force gfp_t)0x80000u) /* Page is reclaimable */

#ifdef CONFIG_KMEMCHECK
/* 不要用 kmemcheck 跟踪 */
#define __GFP_NOTRACK	((__force gfp_t)0x200000u)  /* Don't track with kmemcheck */
#else
#define __GFP_NOTRACK	((__force gfp_t)0)
#endif

/*
 * This may seem redundant, but it's a way of annotating false positives vs.
 * allocations that simply cannot be supported (e.g. page tables).
 */
/*
 * 这可能看起来多余，但它是一种标注的方式，区分误报与
 * 纯粹无法支持的分配（例如，页表）。
 */

// 类型标志

#define __GFP_NOTRACK_FALSE_POSITIVE (__GFP_NOTRACK)

#define __GFP_BITS_SHIFT 22	/* Room for 22 __GFP_FOO bits */
#define __GFP_BITS_MASK ((__force gfp_t)((1 << __GFP_BITS_SHIFT) - 1))

/* This equals 0, but use constants in case they ever change */
// 与GFP_ATOMIC类似，不同之处在于调用不会退给紧急内存池。这就增加了内存分配失败的可能性。
#define GFP_NOWAIT	(GFP_ATOMIC & ~__GFP_HIGH)
/* GFP_ATOMIC means both !wait (__GFP_WAIT not set) and use emergency pool */
/* GFP_ATOMIC 表示既不等待（未设置 __GFP_WAIT）又使用紧急池 */
// 此标志在中断处理程序、下半部、持有自旋锁以及其他不能睡眠的地方
#define GFP_ATOMIC	(__GFP_HIGH)
// 这种分配可以阻塞，但不 会启动磁盘I/O。这个标志在不能引发更多磁盘I/O时能阻塞I/O代码，可能导致令人不愉快的递归。
#define GFP_NOIO	(__GFP_WAIT)
// 这种地分配在必要时可以阻塞，也可以启动磁盘I/O，但不会启动文件系统操作。这个标志在你不能再启动另一个文件系统的操作时，用在文件系统的部分代码中
#define GFP_NOFS	(__GFP_WAIT | __GFP_IO)
// 常规分配方式，可能阻塞。这个标志在睡眠安全时用在进程上下文代码中。为了获得调用者所需内存，内核会尽力而为。
#define GFP_KERNEL	(__GFP_WAIT | __GFP_IO | __GFP_FS)
#define GFP_TEMPORARY	(__GFP_WAIT | __GFP_IO | __GFP_FS | \
			 __GFP_RECLAIMABLE)
// 这是一种常规分配方式，可能阻塞。用于为用户空间进程分配内存时。
#define GFP_USER	(__GFP_WAIT | __GFP_IO | __GFP_FS | __GFP_HARDWALL)
// 这是从ZONE_HIGHMEM进行分配，可能会阻塞。这个标志为用户空间进程分配内存。
#define GFP_HIGHUSER	(__GFP_WAIT | __GFP_IO | __GFP_FS | __GFP_HARDWALL | \
			 __GFP_HIGHMEM)
#define GFP_HIGHUSER_MOVABLE	(__GFP_WAIT | __GFP_IO | __GFP_FS | \
				 __GFP_HARDWALL | __GFP_HIGHMEM | \
				 __GFP_MOVABLE)
#define GFP_IOFS	(__GFP_IO | __GFP_FS)

#ifdef CONFIG_NUMA
#define GFP_THISNODE	(__GFP_THISNODE | __GFP_NOWARN | __GFP_NORETRY)
#else
#define GFP_THISNODE	((__force gfp_t)0)
#endif

/* This mask makes up all the page movable related flags */
#define GFP_MOVABLE_MASK (__GFP_RECLAIMABLE|__GFP_MOVABLE)

/* Control page allocator reclaim behavior */
#define GFP_RECLAIM_MASK (__GFP_WAIT|__GFP_HIGH|__GFP_IO|__GFP_FS|\
			__GFP_NOWARN|__GFP_REPEAT|__GFP_NOFAIL|\
			__GFP_NORETRY|__GFP_NOMEMALLOC)

/* Control slab gfp mask during early boot */
#define GFP_BOOT_MASK __GFP_BITS_MASK & ~(__GFP_WAIT|__GFP_IO|__GFP_FS)

/* Control allocation constraints */
#define GFP_CONSTRAINT_MASK (__GFP_HARDWALL|__GFP_THISNODE)

/* Do not use these with a slab allocator */
#define GFP_SLAB_BUG_MASK (__GFP_DMA32|__GFP_HIGHMEM|~__GFP_BITS_MASK)

/* Flag - indicates that the buffer will be suitable for DMA.  Ignored on some
   platforms, used as appropriate on others */

// 这是从ZONE_DMA进行分配。需要获取能提供DMA使用的内存的设备驱动程序使用这个标志，通常与某个标志一起使用（GFP_ATOMIC和GFP_KERNEL）。
#define GFP_DMA		__GFP_DMA

/* 4GB DMA on some platforms */
#define GFP_DMA32	__GFP_DMA32

#endif