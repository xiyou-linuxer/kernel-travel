#include <linux/memblock.h>
#include <linux/init.h>
#include <linux/stdio.h>
#include <asm/bootinfo.h>
#include <asm/boot_param.h>

const struct loongarchlist_mem_map lalist_mem_map = {
    .map_count = 2,  // 内存映射项数量为2
    .map = {
        {
            .mem_type = 0,
            .mem_start = 0x9000000000000000,
            .mem_size = 0x10000000,
        },
        {
            .mem_type = 0,
            .mem_start = 0x9000000090000000,
            .mem_size = 0x30000000,
        }
    }
};


void __init memblock_init(void)
{
	u32 mem_cnt = lalist_mem_map.map_count;
	u64 mem_start, mem_end, mem_size;
	mem_start = lalist_mem_map.map[0].mem_start;
	mem_size = lalist_mem_map.map[0].mem_size;
	mem_end = mem_start + mem_size;

	for(u32 i = 0; i < mem_cnt; i++)
		memblock_add(mem_start,mem_size);
}