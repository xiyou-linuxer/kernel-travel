#include <asm/page.h>
#include <asm/loongarch.h>
#include <asm/numa.h>
#include <xkernel/memblock.h>

struct pglist_data node_data[MAX_NUMNODES];

char * const zone_names[MAX_NR_ZONES] = {
	 "DMA",
	 "Normal",
	 "Movable",
};

void paging_init(void)
{
	unsigned int node = 0;
	unsigned long zones_size[MAX_NR_ZONES] = {0, 0, 0};
	unsigned long start_pfn, end_pfn;

	/**
	 * NUMA下要处理每个node，这里只有一个node，简化处理
	 */
	get_pfn_range_for_nid(node, &start_pfn, &end_pfn);

	if (end_pfn > max_low_pfn)
		max_low_pfn = end_pfn;

	zones_size[ZONE_NORMAL] = max_low_pfn;
	
	free_area_init(zones_size);
}
