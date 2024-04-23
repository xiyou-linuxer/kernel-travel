#include <linux/memblock.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stdio.h>
#include <linux/printk.h>
#include <debug.h>
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

/* 保证物理地址的合法性 */
static inline phys_addr_t memblock_fit_size(phys_addr_t base, phys_addr_t* size)
{
	return *size = *size < (PHYS_ADDR_MAX - base) ? *size : (PHYS_ADDR_MAX - base);
}
#define for_first_empty_memory_regions(regions)		\
	for()
static int __meminit memblock_add_range(struct memblock_type* memblock_type,phys_addr_t base,phys_addr_t size,enum memblock_flags flags)
{
	phys_addr_t end = base + memblock_fit_size(base,&size);
	int empty_regions_index = -1;

	for(int i = 0;i < INIT_MEMBLOCK_MEMORY_REGIONS;++i) {
		if(memblock_type->regions[i].size == 0) {
			empty_regions_index = i;
			break;
		}
	}

	if(memblock_type->regions[0].size == 0)
		ASSERT(memblock_type->cnt == 1 || memblock_type->total_size == 0);
	memblock_type->regions[empty_regions_index].base = base;
	memblock_type->regions[empty_regions_index].size = size;
	memblock_type->regions[empty_regions_index].flags = flags;
	memblock_type->total_size += size;

	return 0;
}

int __meminit memblock_reserve(phys_addr_t base, phys_addr_t size)
{
	phys_addr_t end = base + size - 1;

	memblock_dbg("%s: [%pa-%pa] %pS\n", __func__,
		     &base, &end, (void *)_RET_IP_);

	return memblock_add_range(&memblock.reserved, base, size, 0);
}

int __meminit memblock_add(phys_addr_t base, phys_addr_t size)
{
	// kbuild 实现可以加对应宏
	phys_addr_t end = base + size - 1;
	memblock_debug = 1;
	memblock_dbg("%s: [0x%pa-0x%pa] 0x%p\n", __func__,&base, &end, (void *)_RET_IP_);
	return memblock_add_range(&memblock.memory,base,size,0);
}