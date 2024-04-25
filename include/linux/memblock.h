#ifndef _LINUX_MEMBLOCK_H
#define _LINUX_MEMBLOCK_H

#include <linux/types.h>


#define pr_fmt(fmt) "memory: " fmt

enum memblock_flags {
	MEMBLOCK_NONE		= 0x0,	/* No special request */
	MEMBLOCK_HOTPLUG	= 0x1,	/* hotpluggable region */
	MEMBLOCK_MIRROR		= 0x2,	/* mirrored region */
	MEMBLOCK_NOMAP		= 0x4,	/* don't add to kernel direct mapping */
	MEMBLOCK_DRIVER_MANAGED = 0x8,	/* always detected via a driver */
	MEMBLOCK_RSRV_NOINIT	= 0x10,	/* don't initialize struct pages */
};

struct memblock_region {
	phys_addr_t base;
	phys_addr_t size;
	enum memblock_flags flags;
};

struct memblock_type {
	unsigned long cnt;
	unsigned long max;
	phys_addr_t total_size;
	struct memblock_region *regions;
	char *name;
};

struct memblock {
	bool bottom_up;  /* is bottom up direction? */
	phys_addr_t current_limit;
	struct memblock_type memory;
	struct memblock_type reserved;
};

void memblock_init(void);
int memblock_add(phys_addr_t base, phys_addr_t size);

extern struct memblock memblock;

#endif
