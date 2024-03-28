#ifndef _LINUX_EFI_H
#define _LINUX_EFI_H

#include <linux/types.h>
#include <linux/compiler_attributes.h>
#include <linux/range.h>
#include <linux/screen_info.h>
#include <linux/string.h>
#include <linux/atomic-instrumented.h>
#include <linux/size.h>
#include <linux/math.h>
#include <linux/align.h>

#include <asm/page.h>

#define __efiapi

#define EFI_SUCCESS		0
#define EFI_LOAD_ERROR		( 1 | (1UL << (BITS_PER_LONG-1)))
#define EFI_INVALID_PARAMETER	( 2 | (1UL << (BITS_PER_LONG-1)))
#define EFI_UNSUPPORTED		( 3 | (1UL << (BITS_PER_LONG-1)))
#define EFI_BAD_BUFFER_SIZE	( 4 | (1UL << (BITS_PER_LONG-1)))
#define EFI_BUFFER_TOO_SMALL	( 5 | (1UL << (BITS_PER_LONG-1)))
#define EFI_NOT_READY		( 6 | (1UL << (BITS_PER_LONG-1)))
#define EFI_DEVICE_ERROR	( 7 | (1UL << (BITS_PER_LONG-1)))
#define EFI_WRITE_PROTECTED	( 8 | (1UL << (BITS_PER_LONG-1)))
#define EFI_OUT_OF_RESOURCES	( 9 | (1UL << (BITS_PER_LONG-1)))
#define EFI_NOT_FOUND		(14 | (1UL << (BITS_PER_LONG-1)))
#define EFI_TIMEOUT		(18 | (1UL << (BITS_PER_LONG-1)))
#define EFI_ABORTED		(21 | (1UL << (BITS_PER_LONG-1)))
#define EFI_SECURITY_VIOLATION	(26 | (1UL << (BITS_PER_LONG-1)))

typedef unsigned long efi_status_t;
typedef u8 efi_bool_t;
typedef u16 efi_char16_t;
typedef u64 efi_physical_addr_t;
typedef void *efi_handle_t;

#define __efiapi

typedef guid_t efi_guid_t __aligned(__alignof__(u32));

#define EFI_GUID(a, b, c, d...) (efi_guid_t){ {					\
	(a) & 0xff, ((a) >> 8) & 0xff, ((a) >> 16) & 0xff, ((a) >> 24) & 0xff,	\
	(b) & 0xff, ((b) >> 8) & 0xff,						\
	(c) & 0xff, ((c) >> 8) & 0xff, d } }

/*
 * Generic EFI table header
 */
typedef	struct {
	u64 signature;
	u32 revision;
	u32 headersize;
	u32 crc32;
	u32 reserved;
} efi_table_hdr_t;

/*
 * Memory map descriptor:
 */

/* Memory types: */
#define EFI_RESERVED_TYPE		 0
#define EFI_LOADER_CODE			 1
#define EFI_LOADER_DATA			 2
#define EFI_BOOT_SERVICES_CODE		 3
#define EFI_BOOT_SERVICES_DATA		 4
#define EFI_RUNTIME_SERVICES_CODE	 5
#define EFI_RUNTIME_SERVICES_DATA	 6
#define EFI_CONVENTIONAL_MEMORY		 7
#define EFI_UNUSABLE_MEMORY		 8
#define EFI_ACPI_RECLAIM_MEMORY		 9
#define EFI_ACPI_MEMORY_NVS		10
#define EFI_MEMORY_MAPPED_IO		11
#define EFI_MEMORY_MAPPED_IO_PORT_SPACE	12
#define EFI_PAL_CODE			13
#define EFI_PERSISTENT_MEMORY		14
#define EFI_MAX_MEMORY_TYPE		15

/* Attribute values: */
#define EFI_MEMORY_UC		((u64)0x0000000000000001ULL)	/* uncached */
#define EFI_MEMORY_WC		((u64)0x0000000000000002ULL)	/* write-coalescing */
#define EFI_MEMORY_WT		((u64)0x0000000000000004ULL)	/* write-through */
#define EFI_MEMORY_WB		((u64)0x0000000000000008ULL)	/* write-back */
#define EFI_MEMORY_UCE		((u64)0x0000000000000010ULL)	/* uncached, exported */
#define EFI_MEMORY_WP		((u64)0x0000000000001000ULL)	/* write-protect */
#define EFI_MEMORY_RP		((u64)0x0000000000002000ULL)	/* read-protect */
#define EFI_MEMORY_XP		((u64)0x0000000000004000ULL)	/* execute-protect */
#define EFI_MEMORY_NV		((u64)0x0000000000008000ULL)	/* non-volatile */
#define EFI_MEMORY_MORE_RELIABLE \
				((u64)0x0000000000010000ULL)	/* higher reliability */
#define EFI_MEMORY_RO		((u64)0x0000000000020000ULL)	/* read-only */
#define EFI_MEMORY_SP		((u64)0x0000000000040000ULL)	/* soft reserved */
#define EFI_MEMORY_CPU_CRYPTO	((u64)0x0000000000080000ULL)	/* supports encryption */
#define EFI_MEMORY_RUNTIME	((u64)0x8000000000000000ULL)	/* range requires runtime mapping */
#define EFI_MEMORY_DESCRIPTOR_VERSION	1

#define EFI_PAGE_SHIFT		12
#define EFI_PAGE_SIZE		(1UL << EFI_PAGE_SHIFT)
#define EFI_PAGES_MAX		(U64_MAX >> EFI_PAGE_SHIFT)

typedef struct {
	u32 type;
	u32 pad;
	u64 phys_addr;
	u64 virt_addr;
	u64 num_pages;
	u64 attribute;
} efi_memory_desc_t;

typedef struct {
	efi_guid_t guid;
	u32 headersize;
	u32 flags;
	u32 imagesize;
} efi_capsule_header_t;

/* EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER */
struct efi_manage_capsule_header {
	u32 ver;
	u16 emb_drv_cnt;
	u16 payload_cnt;
	/*
	 * Variable-size array of the size given by the sum of
	 * emb_drv_cnt and payload_cnt.
	 */
	u64 offset_list[];
} __packed;

/* EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER */
struct efi_manage_capsule_image_header {
	u32 ver;
	efi_guid_t image_type_id;
	u8 image_index;
	u8 reserved_bytes[3];
	u32 image_size;
	u32 vendor_code_size;
	/* hw_ins was introduced in version 2 */
	u64 hw_ins;
	/* capsule_support was introduced in version 3 */
	u64 capsule_support;
} __packed;

/* WIN_CERTIFICATE */
struct win_cert {
	u32 len;
	u16 rev;
	u16 cert_type;
};

/* WIN_CERTIFICATE_UEFI_GUID */
struct win_cert_uefi_guid {
	struct win_cert	hdr;
	efi_guid_t cert_type;
	u8 cert_data[];
};

/* EFI_FIRMWARE_IMAGE_AUTHENTICATION */
struct efi_image_auth {
	u64 mon_count;
	struct win_cert_uefi_guid auth_info;
};

/*
 * EFI capsule flags
 */
#define EFI_CAPSULE_PERSIST_ACROSS_RESET	0x00010000
#define EFI_CAPSULE_POPULATE_SYSTEM_TABLE	0x00020000
#define EFI_CAPSULE_INITIATE_RESET		0x00040000

struct capsule_info {
	efi_capsule_header_t	header;
	efi_capsule_header_t	*capsule;
	int			reset_type;
	long			index;
	size_t			count;
	size_t			total_size;
	struct page		**pages;
	phys_addr_t		*phys;
	size_t			page_bytes_remain;
};

// int efi_capsule_setup_info(struct capsule_info *cap_info, void *kbuff,
//                            size_t hdr_bytes);
// int __efi_capsule_setup_info(struct capsule_info *cap_info);

/*
 * Types and defines for Time Services
 */
#define EFI_TIME_ADJUST_DAYLIGHT 0x1
#define EFI_TIME_IN_DAYLIGHT     0x2
#define EFI_UNSPECIFIED_TIMEZONE 0x07ff

typedef struct {
	u16 year;
	u8 month;
	u8 day;
	u8 hour;
	u8 minute;
	u8 second;
	u8 pad1;
	u32 nanosecond;
	s16 timezone;
	u8 daylight;
	u8 pad2;
} efi_time_t;

typedef struct {
	u32 resolution;
	u32 accuracy;
	u8 sets_to_zero;
} efi_time_cap_t;

typedef union efi_boot_services efi_boot_services_t;

/*
 * Types and defines for EFI ResetSystem
 */
#define EFI_RESET_COLD 0
#define EFI_RESET_WARM 1
#define EFI_RESET_SHUTDOWN 2

/*
 * EFI Runtime Services table
 */
#define EFI_RUNTIME_SERVICES_SIGNATURE ((u64)0x5652453544e5552ULL)
#define EFI_RUNTIME_SERVICES_REVISION  0x00010000

typedef struct {
	efi_table_hdr_t hdr;
	u32 get_time;
	u32 set_time;
	u32 get_wakeup_time;
	u32 set_wakeup_time;
	u32 set_virtual_address_map;
	u32 convert_pointer;
	u32 get_variable;
	u32 get_next_variable;
	u32 set_variable;
	u32 get_next_high_mono_count;
	u32 reset_system;
	u32 update_capsule;
	u32 query_capsule_caps;
	u32 query_variable_info;
} efi_runtime_services_32_t;

typedef efi_status_t efi_get_time_t (efi_time_t *tm, efi_time_cap_t *tc);
typedef efi_status_t efi_set_time_t (efi_time_t *tm);
typedef efi_status_t efi_get_wakeup_time_t (efi_bool_t *enabled, efi_bool_t *pending,
					    efi_time_t *tm);
typedef efi_status_t efi_set_wakeup_time_t (efi_bool_t enabled, efi_time_t *tm);
typedef efi_status_t efi_get_variable_t (efi_char16_t *name, efi_guid_t *vendor, u32 *attr,
					 unsigned long *data_size, void *data);
typedef efi_status_t efi_get_next_variable_t (unsigned long *name_size, efi_char16_t *name,
					      efi_guid_t *vendor);
typedef efi_status_t efi_set_variable_t (efi_char16_t *name, efi_guid_t *vendor, 
					 u32 attr, unsigned long data_size,
					 void *data);
typedef efi_status_t efi_get_next_high_mono_count_t (u32 *count);
typedef void efi_reset_system_t (int reset_type, efi_status_t status,
				 unsigned long data_size, efi_char16_t *data);
typedef efi_status_t efi_set_virtual_address_map_t (unsigned long memory_map_size,
						unsigned long descriptor_size,
						u32 descriptor_version,
						efi_memory_desc_t *virtual_map);
typedef efi_status_t efi_query_variable_info_t(u32 attr,
					       u64 *storage_space,
					       u64 *remaining_space,
					       u64 *max_variable_size);
typedef efi_status_t efi_update_capsule_t(efi_capsule_header_t **capsules,
					  unsigned long count,
					  unsigned long sg_list);
typedef efi_status_t efi_query_capsule_caps_t(efi_capsule_header_t **capsules,
					      unsigned long count,
					      u64 *max_size,
					      int *reset_type);
typedef efi_status_t efi_query_variable_store_t(u32 attributes,
						unsigned long size,
						bool nonblocking);

typedef union {
	struct {
		efi_table_hdr_t				hdr;
		efi_get_time_t __efiapi			*get_time;
		efi_set_time_t __efiapi			*set_time;
		efi_get_wakeup_time_t __efiapi		*get_wakeup_time;
		efi_set_wakeup_time_t __efiapi		*set_wakeup_time;
		efi_set_virtual_address_map_t __efiapi	*set_virtual_address_map;
		void					*convert_pointer;
		efi_get_variable_t __efiapi		*get_variable;
		efi_get_next_variable_t __efiapi	*get_next_variable;
		efi_set_variable_t __efiapi		*set_variable;
		efi_get_next_high_mono_count_t __efiapi	*get_next_high_mono_count;
		efi_reset_system_t __efiapi		*reset_system;
		efi_update_capsule_t __efiapi		*update_capsule;
		efi_query_capsule_caps_t __efiapi	*query_capsule_caps;
		efi_query_variable_info_t __efiapi	*query_variable_info;
	};
	efi_runtime_services_32_t mixed_mode;
} efi_runtime_services_t;

// void efi_native_runtime_setup(void);

/*
 * EFI Configuration Table and GUID definitions
 *
 * These are all defined in a single line to make them easier to
 * grep for and to see them at a glance - while still having a
 * similar structure to the definitions in the spec.
 *
 * Here's how they are structured:
 *
 * GUID: 12345678-1234-1234-1234-123456789012
 * Spec:
 *      #define EFI_SOME_PROTOCOL_GUID \
 *        {0x12345678,0x1234,0x1234,\
 *          {0x12,0x34,0x12,0x34,0x56,0x78,0x90,0x12}}
 * Here:
 *	#define SOME_PROTOCOL_GUID		EFI_GUID(0x12345678, 0x1234, 0x1234,  0x12, 0x34, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12)
 *					^ tabs					    ^extra space
 *
 * Note that the 'extra space' separates the values at the same place
 * where the UEFI SPEC breaks the line.
 */
#define NULL_GUID				EFI_GUID(0x00000000, 0x0000, 0x0000,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
#define MPS_TABLE_GUID				EFI_GUID(0xeb9d2d2f, 0x2d88, 0x11d3,  0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d)
#define ACPI_TABLE_GUID				EFI_GUID(0xeb9d2d30, 0x2d88, 0x11d3,  0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d)
#define ACPI_20_TABLE_GUID			EFI_GUID(0x8868e871, 0xe4f1, 0x11d3,  0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81)
#define SMBIOS_TABLE_GUID			EFI_GUID(0xeb9d2d31, 0x2d88, 0x11d3,  0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d)
#define SMBIOS3_TABLE_GUID			EFI_GUID(0xf2fd1544, 0x9794, 0x4a2c,  0x99, 0x2e, 0xe5, 0xbb, 0xcf, 0x20, 0xe3, 0x94)
#define SAL_SYSTEM_TABLE_GUID			EFI_GUID(0xeb9d2d32, 0x2d88, 0x11d3,  0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d)
#define HCDP_TABLE_GUID				EFI_GUID(0xf951938d, 0x620b, 0x42ef,  0x82, 0x79, 0xa8, 0x4b, 0x79, 0x61, 0x78, 0x98)
#define UGA_IO_PROTOCOL_GUID			EFI_GUID(0x61a4d49e, 0x6f68, 0x4f1b,  0xb9, 0x22, 0xa8, 0x6e, 0xed, 0x0b, 0x07, 0xa2)
#define EFI_GLOBAL_VARIABLE_GUID		EFI_GUID(0x8be4df61, 0x93ca, 0x11d2,  0xaa, 0x0d, 0x00, 0xe0, 0x98, 0x03, 0x2b, 0x8c)
#define UV_SYSTEM_TABLE_GUID			EFI_GUID(0x3b13a7d4, 0x633e, 0x11dd,  0x93, 0xec, 0xda, 0x25, 0x56, 0xd8, 0x95, 0x93)
#define LINUX_EFI_CRASH_GUID			EFI_GUID(0xcfc8fc79, 0xbe2e, 0x4ddc,  0x97, 0xf0, 0x9f, 0x98, 0xbf, 0xe2, 0x98, 0xa0)
#define LOADED_IMAGE_PROTOCOL_GUID		EFI_GUID(0x5b1b31a1, 0x9562, 0x11d2,  0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b)
#define LOADED_IMAGE_DEVICE_PATH_PROTOCOL_GUID	EFI_GUID(0xbc62157e, 0x3e33, 0x4fec,  0x99, 0x20, 0x2d, 0x3b, 0x36, 0xd7, 0x50, 0xdf)
#define EFI_DEVICE_PATH_PROTOCOL_GUID		EFI_GUID(0x09576e91, 0x6d3f, 0x11d2,  0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b)
#define EFI_DEVICE_PATH_TO_TEXT_PROTOCOL_GUID	EFI_GUID(0x8b843e20, 0x8132, 0x4852,  0x90, 0xcc, 0x55, 0x1a, 0x4e, 0x4a, 0x7f, 0x1c)
#define EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL_GUID	EFI_GUID(0x05c99a21, 0xc70f, 0x4ad2,  0x8a, 0x5f, 0x35, 0xdf, 0x33, 0x43, 0xf5, 0x1e)
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID	EFI_GUID(0x9042a9de, 0x23dc, 0x4a38,  0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a)
#define EFI_UGA_PROTOCOL_GUID			EFI_GUID(0x982c298b, 0xf4fa, 0x41cb,  0xb8, 0x38, 0x77, 0xaa, 0x68, 0x8f, 0xb8, 0x39)
#define EFI_PCI_IO_PROTOCOL_GUID		EFI_GUID(0x4cf5b200, 0x68b8, 0x4ca5,  0x9e, 0xec, 0xb2, 0x3e, 0x3f, 0x50, 0x02, 0x9a)
#define EFI_FILE_INFO_ID			EFI_GUID(0x09576e92, 0x6d3f, 0x11d2,  0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b)
#define EFI_SYSTEM_RESOURCE_TABLE_GUID		EFI_GUID(0xb122a263, 0x3661, 0x4f68,  0x99, 0x29, 0x78, 0xf8, 0xb0, 0xd6, 0x21, 0x80)
#define EFI_FILE_SYSTEM_GUID			EFI_GUID(0x964e5b22, 0x6459, 0x11d2,  0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b)
#define DEVICE_TREE_GUID			EFI_GUID(0xb1b621d5, 0xf19c, 0x41a5,  0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0)
#define EFI_PROPERTIES_TABLE_GUID		EFI_GUID(0x880aaca3, 0x4adc, 0x4a04,  0x90, 0x79, 0xb7, 0x47, 0x34, 0x08, 0x25, 0xe5)
#define EFI_RNG_PROTOCOL_GUID			EFI_GUID(0x3152bca5, 0xeade, 0x433d,  0x86, 0x2e, 0xc0, 0x1c, 0xdc, 0x29, 0x1f, 0x44)
#define EFI_RNG_ALGORITHM_RAW			EFI_GUID(0xe43176d7, 0xb6e8, 0x4827,  0xb7, 0x84, 0x7f, 0xfd, 0xc4, 0xb6, 0x85, 0x61)
#define EFI_MEMORY_ATTRIBUTES_TABLE_GUID	EFI_GUID(0xdcfa911d, 0x26eb, 0x469f,  0xa2, 0x20, 0x38, 0xb7, 0xdc, 0x46, 0x12, 0x20)
#define EFI_CONSOLE_OUT_DEVICE_GUID		EFI_GUID(0xd3b36f2c, 0xd551, 0x11d4,  0x9a, 0x46, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d)
#define APPLE_PROPERTIES_PROTOCOL_GUID		EFI_GUID(0x91bd12fe, 0xf6c3, 0x44fb,  0xa5, 0xb7, 0x51, 0x22, 0xab, 0x30, 0x3a, 0xe0)
#define EFI_TCG2_PROTOCOL_GUID			EFI_GUID(0x607f766c, 0x7455, 0x42be,  0x93, 0x0b, 0xe4, 0xd7, 0x6d, 0xb2, 0x72, 0x0f)
#define EFI_LOAD_FILE_PROTOCOL_GUID		EFI_GUID(0x56ec3091, 0x954c, 0x11d2,  0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b)
#define EFI_LOAD_FILE2_PROTOCOL_GUID		EFI_GUID(0x4006c0c1, 0xfcb3, 0x403e,  0x99, 0x6d, 0x4a, 0x6c, 0x87, 0x24, 0xe0, 0x6d)
#define EFI_RT_PROPERTIES_TABLE_GUID		EFI_GUID(0xeb66918a, 0x7eef, 0x402a,  0x84, 0x2e, 0x93, 0x1d, 0x21, 0xc3, 0x8a, 0xe9)
#define EFI_DXE_SERVICES_TABLE_GUID		EFI_GUID(0x05ad34ba, 0x6f02, 0x4214,  0x95, 0x2e, 0x4d, 0xa0, 0x39, 0x8e, 0x2b, 0xb9)
#define EFI_SMBIOS_PROTOCOL_GUID		EFI_GUID(0x03583ff6, 0xcb36, 0x4940,  0x94, 0x7e, 0xb9, 0xb3, 0x9f, 0x4a, 0xfa, 0xf7)

#define EFI_IMAGE_SECURITY_DATABASE_GUID	EFI_GUID(0xd719b2cb, 0x3d3a, 0x4596,  0xa3, 0xbc, 0xda, 0xd0, 0x0e, 0x67, 0x65, 0x6f)
#define EFI_SHIM_LOCK_GUID			EFI_GUID(0x605dab50, 0xe046, 0x4300,  0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23)

#define EFI_CERT_SHA256_GUID			EFI_GUID(0xc1c41626, 0x504c, 0x4092, 0xac, 0xa9, 0x41, 0xf9, 0x36, 0x93, 0x43, 0x28)
#define EFI_CERT_X509_GUID			EFI_GUID(0xa5c059a1, 0x94e4, 0x4aa7, 0x87, 0xb5, 0xab, 0x15, 0x5c, 0x2b, 0xf0, 0x72)
#define EFI_CERT_X509_SHA256_GUID		EFI_GUID(0x3bd2a492, 0x96c0, 0x4079, 0xb4, 0x20, 0xfc, 0xf9, 0x8e, 0xf1, 0x03, 0xed)
#define EFI_CC_BLOB_GUID			EFI_GUID(0x067b1f5f, 0xcf26, 0x44c5, 0x85, 0x54, 0x93, 0xd7, 0x77, 0x91, 0x2d, 0x42)

/*
 * This GUID is used to pass to the kernel proper the struct screen_info
 * structure that was populated by the stub based on the GOP protocol instance
 * associated with ConOut
 */
#define LINUX_EFI_SCREEN_INFO_TABLE_GUID	EFI_GUID(0xe03fc20a, 0x85dc, 0x406e,  0xb9, 0x0e, 0x4a, 0xb5, 0x02, 0x37, 0x1d, 0x95)
#define LINUX_EFI_ARM_CPU_STATE_TABLE_GUID	EFI_GUID(0xef79e4aa, 0x3c3d, 0x4989,  0xb9, 0x02, 0x07, 0xa9, 0x43, 0xe5, 0x50, 0xd2)
#define LINUX_EFI_LOADER_ENTRY_GUID		EFI_GUID(0x4a67b082, 0x0a4c, 0x41cf,  0xb6, 0xc7, 0x44, 0x0b, 0x29, 0xbb, 0x8c, 0x4f)
#define LINUX_EFI_RANDOM_SEED_TABLE_GUID	EFI_GUID(0x1ce1e5bc, 0x7ceb, 0x42f2,  0x81, 0xe5, 0x8a, 0xad, 0xf1, 0x80, 0xf5, 0x7b)
#define LINUX_EFI_TPM_EVENT_LOG_GUID		EFI_GUID(0xb7799cb0, 0xeca2, 0x4943,  0x96, 0x67, 0x1f, 0xae, 0x07, 0xb7, 0x47, 0xfa)
#define LINUX_EFI_TPM_FINAL_LOG_GUID		EFI_GUID(0x1e2ed096, 0x30e2, 0x4254,  0xbd, 0x89, 0x86, 0x3b, 0xbe, 0xf8, 0x23, 0x25)
#define LINUX_EFI_MEMRESERVE_TABLE_GUID		EFI_GUID(0x888eb0c6, 0x8ede, 0x4ff5,  0xa8, 0xf0, 0x9a, 0xee, 0x5c, 0xb9, 0x77, 0xc2)
#define LINUX_EFI_INITRD_MEDIA_GUID		EFI_GUID(0x5568e427, 0x68fc, 0x4f3d,  0xac, 0x74, 0xca, 0x55, 0x52, 0x31, 0xcc, 0x68)
#define LINUX_EFI_MOK_VARIABLE_TABLE_GUID	EFI_GUID(0xc451ed2b, 0x9694, 0x45d3,  0xba, 0xba, 0xed, 0x9f, 0x89, 0x88, 0xa3, 0x89)
#define LINUX_EFI_COCO_SECRET_AREA_GUID		EFI_GUID(0xadf956ad, 0xe98c, 0x484c,  0xae, 0x11, 0xb5, 0x1c, 0x7d, 0x33, 0x64, 0x47)
#define LINUX_EFI_BOOT_MEMMAP_GUID		EFI_GUID(0x800f683f, 0xd08b, 0x423a,  0xa2, 0x93, 0x96, 0x5c, 0x3c, 0x6f, 0xe2, 0xb4)

#define RISCV_EFI_BOOT_PROTOCOL_GUID		EFI_GUID(0xccd15fec, 0x6f73, 0x4eec,  0x83, 0x95, 0x3e, 0x69, 0xe4, 0xb9, 0x40, 0xbf)

/*
 * This GUID may be installed onto the kernel image's handle as a NULL protocol
 * to signal to the stub that the placement of the image should be respected,
 * and moving the image in physical memory is undesirable. To ensure
 * compatibility with 64k pages kernels with virtually mapped stacks, and to
 * avoid defeating physical randomization, this protocol should only be
 * installed if the image was placed at a randomized 128k aligned address in
 * memory.
 */
#define LINUX_EFI_LOADED_IMAGE_FIXED_GUID	EFI_GUID(0xf5a37b6d, 0x3344, 0x42a5,  0xb6, 0xbb, 0x97, 0x86, 0x48, 0xc1, 0x89, 0x0a)

/* OEM GUIDs */
#define DELLEMC_EFI_RCI2_TABLE_GUID		EFI_GUID(0x2d9f28a2, 0xa886, 0x456a,  0x97, 0xa8, 0xf1, 0x1e, 0xf2, 0x4f, 0xf4, 0x55)
#define AMD_SEV_MEM_ENCRYPT_GUID		EFI_GUID(0x0cf29b71, 0x9e51, 0x433a,  0xa3, 0xb7, 0x81, 0xf3, 0xab, 0x16, 0xb8, 0x75)

typedef struct {
	efi_guid_t guid;
	u64 table;
} efi_config_table_64_t;

typedef struct {
	efi_guid_t guid;
	u32 table;
} efi_config_table_32_t;

typedef union {
	struct {
		efi_guid_t guid;
		void *table;
	};
	efi_config_table_32_t mixed_mode;
} efi_config_table_t;

typedef struct {
	efi_guid_t guid;
	unsigned long *ptr;
	const char name[16];
} efi_config_table_type_t;

#define EFI_SYSTEM_TABLE_SIGNATURE ((u64)0x5453595320494249ULL)
#define EFI_DXE_SERVICES_TABLE_SIGNATURE ((u64)0x565245535f455844ULL)

#define EFI_2_30_SYSTEM_TABLE_REVISION  ((2 << 16) | (30))
#define EFI_2_20_SYSTEM_TABLE_REVISION  ((2 << 16) | (20))
#define EFI_2_10_SYSTEM_TABLE_REVISION  ((2 << 16) | (10))
#define EFI_2_00_SYSTEM_TABLE_REVISION  ((2 << 16) | (00))
#define EFI_1_10_SYSTEM_TABLE_REVISION  ((1 << 16) | (10))
#define EFI_1_02_SYSTEM_TABLE_REVISION  ((1 << 16) | (02))

typedef struct {
	efi_table_hdr_t hdr;
	u64 fw_vendor;	/* physical addr of CHAR16 vendor string */
	u32 fw_revision;
	u32 __pad1;
	u64 con_in_handle;
	u64 con_in;
	u64 con_out_handle;
	u64 con_out;
	u64 stderr_handle;
	u64 stderr;
	u64 runtime;
	u64 boottime;
	u32 nr_tables;
	u32 __pad2;
	u64 tables;
} efi_system_table_64_t;

typedef struct {
	efi_table_hdr_t hdr;
	u32 fw_vendor;	/* physical addr of CHAR16 vendor string */
	u32 fw_revision;
	u32 con_in_handle;
	u32 con_in;
	u32 con_out_handle;
	u32 con_out;
	u32 stderr_handle;
	u32 stderr;
	u32 runtime;
	u32 boottime;
	u32 nr_tables;
	u32 tables;
} efi_system_table_32_t;

typedef union efi_simple_text_input_protocol efi_simple_text_input_protocol_t;
typedef union efi_simple_text_output_protocol efi_simple_text_output_protocol_t;

typedef union {
	struct {
		efi_table_hdr_t hdr;
		unsigned long fw_vendor;	/* physical addr of CHAR16 vendor string */
		u32 fw_revision;
		unsigned long con_in_handle;
		efi_simple_text_input_protocol_t *con_in;
		unsigned long con_out_handle;
		efi_simple_text_output_protocol_t *con_out;
		unsigned long stderr_handle;
		unsigned long stderr;
		efi_runtime_services_t *runtime;
		efi_boot_services_t *boottime;
		unsigned long nr_tables;
		unsigned long tables;
	};
	efi_system_table_32_t mixed_mode;
} efi_system_table_t;

struct efi_boot_memmap {
	unsigned long		map_size;
	unsigned long		desc_size;
	u32			desc_ver;
	unsigned long		map_key;
	unsigned long		buff_size;
	efi_memory_desc_t	map[];
};

/*
 * Architecture independent structure for describing a memory map for the
 * benefit of efi_memmap_init_early(), and for passing context between
 * efi_memmap_alloc() and efi_memmap_install().
 */
struct efi_memory_map_data {
	phys_addr_t phys_map;
	unsigned long size;
	unsigned long desc_version;
	unsigned long desc_size;
	unsigned long flags;
};

struct efi_memory_map {
	phys_addr_t phys_map;
	void *map;
	void *map_end;
	int nr_map;
	unsigned long desc_version;
	unsigned long desc_size;
#define EFI_MEMMAP_LATE (1UL << 0)
#define EFI_MEMMAP_MEMBLOCK (1UL << 1)
#define EFI_MEMMAP_SLAB (1UL << 2)
	unsigned long flags;
};

struct efi_mem_range {
	struct range range;
	u64 attribute;
};

typedef struct {
	u32 version;
	u32 length;
	u64 memory_protection_attribute;
} efi_properties_table_t;

#define EFI_PROPERTIES_TABLE_VERSION	0x00010000
#define EFI_PROPERTIES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA	0x1

typedef struct {
	u16 version;
	u16 length;
	u32 runtime_services_supported;
} efi_rt_properties_table_t;

#define EFI_RT_PROPERTIES_TABLE_VERSION	0x1

#define EFI_INVALID_TABLE_ADDR		(~0UL)

typedef struct {
	u32 version;
	u32 num_entries;
	u32 desc_size;
	u32 reserved;
	efi_memory_desc_t entry[0];
} efi_memory_attributes_table_t;

typedef struct {
	efi_guid_t signature_owner;
	u8 signature_data[];
} efi_signature_data_t;

typedef struct {
	efi_guid_t signature_type;
	u32 signature_list_size;
	u32 signature_header_size;
	u32 signature_size;
	u8 signature_header[];
	/* efi_signature_data_t signatures[][] */
} efi_signature_list_t;

typedef u8 efi_sha256_hash_t[32];

typedef struct {
	efi_sha256_hash_t to_be_signed_hash;
	efi_time_t time_of_revocation;
} efi_cert_x509_sha256_t;

// extern unsigned long __ro_after_init efi_rng_seed;		/* RNG Seed table */

/*
 * All runtime access to EFI goes through this structure:
 */
extern struct efi {
	const efi_runtime_services_t	*runtime;		/* EFI runtime services table */
	unsigned int			runtime_version;	/* Runtime services version */
	unsigned int			runtime_supported_mask;

	unsigned long			acpi;			/* ACPI table  (IA64 ext 0.71) */
	unsigned long			acpi20;			/* ACPI table  (ACPI 2.0) */
	unsigned long			smbios;			/* SMBIOS table (32 bit entry point) */
	unsigned long			smbios3;		/* SMBIOS table (64 bit entry point) */
	unsigned long			esrt;			/* ESRT table */
	unsigned long			tpm_log;		/* TPM2 Event Log table */
	unsigned long			tpm_final_log;		/* TPM2 Final Events Log table */
	unsigned long			mokvar_table;		/* MOK variable config table */
	unsigned long			coco_secret;		/* Confidential computing secret table */

	efi_get_time_t			*get_time;
	efi_set_time_t			*set_time;
	efi_get_wakeup_time_t		*get_wakeup_time;
	efi_set_wakeup_time_t		*set_wakeup_time;
	efi_get_variable_t		*get_variable;
	efi_get_next_variable_t		*get_next_variable;
	efi_set_variable_t		*set_variable;
	efi_set_variable_t		*set_variable_nonblocking;
	efi_query_variable_info_t	*query_variable_info;
	efi_query_variable_info_t	*query_variable_info_nonblocking;
	efi_update_capsule_t		*update_capsule;
	efi_query_capsule_caps_t	*query_capsule_caps;
	efi_get_next_high_mono_count_t	*get_next_high_mono_count;
	efi_reset_system_t		*reset_system;

	struct efi_memory_map		memmap;
	unsigned long			flags;
} efi;

#define EFI_RT_SUPPORTED_GET_TIME				0x0001
#define EFI_RT_SUPPORTED_SET_TIME				0x0002
#define EFI_RT_SUPPORTED_GET_WAKEUP_TIME			0x0004
#define EFI_RT_SUPPORTED_SET_WAKEUP_TIME			0x0008
#define EFI_RT_SUPPORTED_GET_VARIABLE				0x0010
#define EFI_RT_SUPPORTED_GET_NEXT_VARIABLE_NAME			0x0020
#define EFI_RT_SUPPORTED_SET_VARIABLE				0x0040
#define EFI_RT_SUPPORTED_SET_VIRTUAL_ADDRESS_MAP		0x0080
#define EFI_RT_SUPPORTED_CONVERT_POINTER			0x0100
#define EFI_RT_SUPPORTED_GET_NEXT_HIGH_MONOTONIC_COUNT		0x0200
#define EFI_RT_SUPPORTED_RESET_SYSTEM				0x0400
#define EFI_RT_SUPPORTED_UPDATE_CAPSULE				0x0800
#define EFI_RT_SUPPORTED_QUERY_CAPSULE_CAPABILITIES		0x1000
#define EFI_RT_SUPPORTED_QUERY_VARIABLE_INFO			0x2000

#define EFI_RT_SUPPORTED_ALL					0x3fff

#define EFI_RT_SUPPORTED_TIME_SERVICES				0x0003
#define EFI_RT_SUPPORTED_WAKEUP_SERVICES			0x000c
#define EFI_RT_SUPPORTED_VARIABLE_SERVICES			0x0070

// extern struct mm_struct efi_mm;

static inline int
efi_guidcmp (efi_guid_t left, efi_guid_t right)
{
	return memcmp(&left, &right, sizeof (efi_guid_t));
}

struct linux_efi_memreserve {
	int		size;			// allocated size of the array
	atomic_t	count;			// number of entries used
	phys_addr_t	next;			// pa of next struct instance
	struct {
		phys_addr_t	base;
		phys_addr_t	size;
	} entry[];
};

#endif /* _LINUX_EFI_H */
