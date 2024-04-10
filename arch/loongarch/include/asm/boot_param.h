#ifndef _BOOT_PARAM_H
#define _BOOT_PARAM_H

#include <asm/types.h>

/* add necessary defines */
/*typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned long u64;*/
#define __packed                        __attribute__((__packed__))

#define ADDRESS_TYPE_SYSRAM	1
#define ADDRESS_TYPE_RESERVED	2
#define ADDRESS_TYPE_ACPI	3
#define ADDRESS_TYPE_NVS	4
#define ADDRESS_TYPE_PMEM	5

#define LOONGSON3_BOOT_MEM_MAP_MAX 128

#define LOONGSON_MEM_SIGNATURE			"MEM"
#define LOONGSON_VBIOS_SIGNATURE		"VBIOS"
#define LOONGSON_EFIBOOT_SIGNATURE		"BPI"
#define LOONGSON_SCREENINFO_SIGNATURE		"SINFO"
#define LOONGSON_EFIBOOT_VERSION		1000

/* Values for Version BPI */
enum bpi_version {
	BPI_VERSION_NONE = 0,
	BPI_VERSION_V1 = 1000, /* Signature="BPI01000" */
	BPI_VERSION_V2 = 1001, /* Signature="BPI01001" */
};

/* Flags in boot_params_interface */
#define BPI_FLAGS_UEFI_SUPPORTED BIT(0)

struct _extention_list_hdr {
	u64	signature;
	u32	length;
	u8	revision;
	u8	checksum;
	struct	_extention_list_hdr *next;
} __packed;

struct boot_params_interface {
	u64	signature;	/* {"B", "P", "I", "0", "1", ... } */
	void	*systemtable;
	struct	_extention_list_hdr *extlist;
	u64	flags;
} __packed;

struct loongsonlist_mem_map {
	struct	_extention_list_hdr header;	/* {"M", "E", "M"} */
	u8	map_count;
	struct	loongson_mem_map {
		u32 mem_type;
		u64 mem_start;
		u64 mem_size;
	} __packed map[LOONGSON3_BOOT_MEM_MAP_MAX];
} __packed;

struct loongsonlist_vbios {
	struct	_extention_list_hdr header;	/* {"V", "B", "I", "O", "S"} */
	u64	vbios_addr;
} __packed;

struct loongsonlist_screeninfo {
	struct	_extention_list_hdr header;	/* {"S", "I", "N", "F", "O"} */
	//struct	screen_info si;
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
	const char *cpuname;
	int nr_cpus;
	int nr_nodes;
	int cores_per_node;
	int cores_per_package;
	u16 boot_cpu_id;
	u64 reserved_cpus_mask;
	u64 suspend_addr;
	u64 vgabios_addr;
	u64 pm1_evt_reg;
	u64 pm1_ena_reg;
	u64 pm1_cnt_reg;
	u64 gpe0_ena_reg;
	u32 bpi_version;
	u8 pcie_wake_enabled;
	u8 is_soc_cpu;
};

#endif /* _BOOT_PARAM_H */
