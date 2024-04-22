#include <linux/memblock.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stdio.h>
#include <linux/printk.h>

#define INIT_MEMBLOCK_MEMORY_REGIONS 32
#define INIT_MEMBLOCK_RESERVED_REGIONS INIT_MEMBLOCK_MEMORY_REGIONS
#define MEMBLOCK_ALLOC_ANYWHERE	(~(phys_addr_t)0)

#define memblock_dbg(fmt, ...)						\
	do {								\
		if (memblock_debug)					\
			pr_info(fmt, ##__VA_ARGS__);			\
	} while (0)

static struct memblock_region memblock_memory_init_regions[INIT_MEMBLOCK_MEMORY_REGIONS];
static struct memblock_region memblock_reserved_init_regions[INIT_MEMBLOCK_MEMORY_REGIONS];

struct memblock memblock = {
	.memory.regions		= memblock_memory_init_regions,
	.memory.cnt		= 1,	/* empty dummy entry */
	.memory.max		= INIT_MEMBLOCK_MEMORY_REGIONS,
	.memory.name		= "memory",

	.reserved.regions	= memblock_reserved_init_regions,
	.reserved.cnt		= 1,	/* empty dummy entry */
	.reserved.max		= INIT_MEMBLOCK_RESERVED_REGIONS,
	.reserved.name		= "reserved",

	.bottom_up		= false,
	.current_limit		= MEMBLOCK_ALLOC_ANYWHERE,
};



static int memblock_debug __meminitdata;

static void __meminit memblock_add_range(struct memblock_type* memblock_type,phys_addr_t base,phys_addr_t size,enum memblock_flags flags)
{
	// memblock_type.cnt
	if(type->regions[0].size == 0) {
		//...
	}
}

int __meminit memblock_add(phys_addr_t base, phys_addr_t size)
{
	// kbuild 实现可以加对应宏
	memblock_debug = 1;
	phys_addr_t end = base + size - 1;
	memblock_dbg("%s: [%pa-%pa] %p\n", __func__,&base, &end, (void *)_RET_IP_);
	return memblock_add_range(&memblock.memory,base,size,0);
}