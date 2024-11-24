#ifndef _BOOT_PARAM_H
#define _BOOT_PARAM_H

#ifdef CONFIG_VT
#include <linux/screen_info.h>
#endif

#include <asm/types.h>

/* add necessary defines */
#define __packed                        __attribute__((__packed__))
#define LOONGARCH_BOOT_MEM_MAP_MAX 0x10

#define ADDRESS_TYPE_SYSRAM	1
#define ADDRESS_TYPE_RESERVED	2
#define ADDRESS_TYPE_ACPI	3
#define ADDRESS_TYPE_NVS	4
#define ADDRESS_TYPE_PMEM	5

#define EFI_RUNTIME_MAP_START	   100
#define LOONGSON3_BOOT_MEM_MAP_MAX 128

#define LOONGSON_EFIBOOT_SIGNATURE	"BPI"
#define LOONGSON_MEM_SIGNATURE		"MEM"
#define LOONGSON_VBIOS_SIGNATURE	"VBIOS"
#define LOONGSON_SCREENINFO_SIGNATURE	"SINFO"

/* Values for Version BPI */
enum bpi_version {
	BPI_VERSION_V1 = 1000, /* Signature="BPI01000" */
	BPI_VERSION_V2 = 1001, /* Signature="BPI01001" */
	BPI_VERSION_V3 = 1002, /* Signature="BPI01002" */
};

/* Flags in bootparamsinterface */
#define BPI_FLAGS_UEFI_SUPPORTED	BIT(0)
#define BPI_FLAGS_SOC_CPU		BIT(1)

struct _extention_list_hdr {
	u64	signature;
	u32	length;
	u8	revision;
	u8	checksum;
	u64	next_offset;
} __packed;

struct boot_params {
	u64	signature;	/* {"B", "P", "I", "0", "1", ... } */
	void	*systemtable;
	u64	extlist_offset;
	u64 	flags;
} __packed;

struct loongsonlist_mem_map_legacy {
	struct	_extention_list_hdr header;	/* {"M", "E", "M"} */
	u8	map_count;
	struct	loongson_mem_map {
		u32 mem_type;
		u64 mem_start;
		u64 mem_size;
	} __packed map[LOONGSON3_BOOT_MEM_MAP_MAX];
} __packed;

struct loongsonlist_mem_map {
	struct	_extention_list_hdr header;	/* {"M", "E", "M"} */
	u8	map_count;
	u32	desc_version;
	struct efi_mmap {
		u32 mem_type;
		u32 padding;
		u64 mem_start;
		u64 mem_vaddr;
		u64 mem_size;
		u64 attribute;
	} __packed map[LOONGSON3_BOOT_MEM_MAP_MAX];
} __packed;

struct loongsonlist_vbios {
	struct	_extention_list_hdr header;	/* {"V", "B", "I", "O", "S"} */
	u64	vbios_addr;
} __packed;

struct loongsonlist_screeninfo{
	struct	_extention_list_hdr header;	/* {"S", "I", "N", "F", "O"} */
#ifdef CONFIG_VT
	struct	screen_info si;
#endif
} __packed;

struct loongson_board_info {
	int bios_size;
	char *bios_vendor;
	char *bios_version;
	char *bios_release_date;
	char *board_name;
	char *board_vendor;
};

struct loongson_system_configuration {
	int bpi_ver;
	int nr_cpus;
	int nr_nodes;
	int nr_io_pics;
	int boot_cpu_id;
	int cores_per_node;
	int cores_per_package;
	char *cpuname;
	u64 suspend_addr;
	u64 vgabios_addr;
	int is_soc_cpu;
};

/**
 * ============================== old code ==============================
 */

struct loongarchlist_mem_map {
	u8	map_count;
	struct	loongson_mem_map_old {
		u32 mem_type;
		u64 mem_start;
		u64 mem_size;
	} __packed map[LOONGARCH_BOOT_MEM_MAP_MAX];
} __packed;

// 初始化内存映射数组
extern const struct loongarchlist_mem_map lalist_mem_map;

/**
 * ============================== old code ==============================
 */

#endif /* _BOOT_PARAM_H */
