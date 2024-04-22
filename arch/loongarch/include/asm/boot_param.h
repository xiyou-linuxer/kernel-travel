#ifndef _BOOT_PARAM_H
#define _BOOT_PARAM_H

#include <asm/types.h>

/* add necessary defines */
/*typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned long u64;*/
#define __packed                        __attribute__((__packed__))
#define LOONGARCH_BOOT_MEM_MAP_MAX 0x10

struct loongarchlist_mem_map {
	u8	map_count;
	struct	loongson_mem_map {
		u32 mem_type;
		u64 mem_start;
		u64 mem_size;
	} __packed map[LOONGARCH_BOOT_MEM_MAP_MAX];
} __packed;

// 初始化内存映射数组
extern const struct loongarchlist_mem_map lalist_mem_map;

#endif /* _BOOT_PARAM_H */
