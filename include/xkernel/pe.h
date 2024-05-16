/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2011 Red Hat, Inc.
 * All rights reserved.
 *
 * Author(s): Peter Jones <pjones@redhat.com>
 */
#ifndef __LINUX_PE_H
#define __LINUX_PE_H

/*
 * Linux EFI stub v1.0 adds the following functionality:
 * - Loading initrd from the LINUX_EFI_INITRD_MEDIA_GUID device path,
 * - Loading/starting the kernel from firmware that targets a different
 *   machine type, via the entrypoint exposed in the .compat PE/COFF section.
 *
 * The recommended way of loading and starting v1.0 or later kernels is to use
 * the LoadImage() and StartImage() EFI boot services, and expose the initrd
 * via the LINUX_EFI_INITRD_MEDIA_GUID device path.
 *
 * Versions older than v1.0 support initrd loading via the image load options
 * (using initrd=, limited to the volume from which the kernel itself was
 * loaded), or via arch specific means (bootparams, DT, etc).
 *
 * On x86, LoadImage() and StartImage() can be omitted if the EFI handover
 * protocol is implemented, which can be inferred from the version,
 * handover_offset and xloadflags fields in the bootparams structure.
 */
#define LINUX_EFISTUB_MAJOR_VERSION		0x1
#define LINUX_EFISTUB_MINOR_VERSION		0x1

/*
 * LINUX_PE_MAGIC appears at offset 0x38 into the MS-DOS header of EFI bootable
 * Linux kernel images that target the architecture as specified by the PE/COFF
 * header machine type field.
 */
#define LINUX_PE_MAGIC	0x818223cd

#define MZ_MAGIC	0x5a4d	/* "MZ" */

#define PE_MAGIC		0x00004550	/* "PE\0\0" */
#define PE_OPT_MAGIC_PE32	0x010b
#define PE_OPT_MAGIC_PE32_ROM	0x0107
#define PE_OPT_MAGIC_PE32PLUS	0x020b

/* machine type */
#define	IMAGE_FILE_MACHINE_UNKNOWN	0x0000
#define	IMAGE_FILE_MACHINE_AM33		0x01d3
#define	IMAGE_FILE_MACHINE_AMD64	0x8664
#define	IMAGE_FILE_MACHINE_ARM		0x01c0
#define	IMAGE_FILE_MACHINE_ARMV7	0x01c4
#define	IMAGE_FILE_MACHINE_ARM64	0xaa64
#define	IMAGE_FILE_MACHINE_EBC		0x0ebc
#define	IMAGE_FILE_MACHINE_I386		0x014c
#define	IMAGE_FILE_MACHINE_IA64		0x0200
#define	IMAGE_FILE_MACHINE_M32R		0x9041
#define	IMAGE_FILE_MACHINE_MIPS16	0x0266
#define	IMAGE_FILE_MACHINE_MIPSFPU	0x0366
#define	IMAGE_FILE_MACHINE_MIPSFPU16	0x0466
#define	IMAGE_FILE_MACHINE_POWERPC	0x01f0
#define	IMAGE_FILE_MACHINE_POWERPCFP	0x01f1
#define	IMAGE_FILE_MACHINE_R4000	0x0166
#define	IMAGE_FILE_MACHINE_RISCV32	0x5032
#define	IMAGE_FILE_MACHINE_RISCV64	0x5064
#define	IMAGE_FILE_MACHINE_RISCV128	0x5128
#define	IMAGE_FILE_MACHINE_SH3		0x01a2
#define	IMAGE_FILE_MACHINE_SH3DSP	0x01a3
#define	IMAGE_FILE_MACHINE_SH3E		0x01a4
#define	IMAGE_FILE_MACHINE_SH4		0x01a6
#define	IMAGE_FILE_MACHINE_SH5		0x01a8
#define	IMAGE_FILE_MACHINE_THUMB	0x01c2
#define	IMAGE_FILE_MACHINE_WCEMIPSV2	0x0169
#define	IMAGE_FILE_MACHINE_LOONGARCH32	0x6232
#define	IMAGE_FILE_MACHINE_LOONGARCH64	0x6264

/* flags */
#define IMAGE_FILE_RELOCS_STRIPPED           0x0001
#define IMAGE_FILE_EXECUTABLE_IMAGE          0x0002
#define IMAGE_FILE_LINE_NUMS_STRIPPED        0x0004
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED       0x0008
#define IMAGE_FILE_AGGRESSIVE_WS_TRIM        0x0010
#define IMAGE_FILE_LARGE_ADDRESS_AWARE       0x0020
#define IMAGE_FILE_16BIT_MACHINE             0x0040
#define IMAGE_FILE_BYTES_REVERSED_LO         0x0080
#define IMAGE_FILE_32BIT_MACHINE             0x0100
#define IMAGE_FILE_DEBUG_STRIPPED            0x0200
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP   0x0400
#define IMAGE_FILE_NET_RUN_FROM_SWAP         0x0800
#define IMAGE_FILE_SYSTEM                    0x1000
#define IMAGE_FILE_DLL                       0x2000
#define IMAGE_FILE_UP_SYSTEM_ONLY            0x4000
#define IMAGE_FILE_BYTES_REVERSED_HI         0x8000

#define IMAGE_FILE_OPT_ROM_MAGIC	0x107
#define IMAGE_FILE_OPT_PE32_MAGIC	0x10b
#define IMAGE_FILE_OPT_PE32_PLUS_MAGIC	0x20b

#define IMAGE_SUBSYSTEM_UNKNOWN			 0
#define IMAGE_SUBSYSTEM_NATIVE			 1
#define IMAGE_SUBSYSTEM_WINDOWS_GUI		 2
#define IMAGE_SUBSYSTEM_WINDOWS_CUI		 3
#define IMAGE_SUBSYSTEM_POSIX_CUI		 7
#define IMAGE_SUBSYSTEM_WINDOWS_CE_GUI		 9
#define IMAGE_SUBSYSTEM_EFI_APPLICATION		10
#define IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER	11
#define IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER	12
#define IMAGE_SUBSYSTEM_EFI_ROM_IMAGE		13
#define IMAGE_SUBSYSTEM_XBOX			14

#define IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE          0x0040
#define IMAGE_DLL_CHARACTERISTICS_FORCE_INTEGRITY       0x0080
#define IMAGE_DLL_CHARACTERISTICS_NX_COMPAT             0x0100
#define IMAGE_DLLCHARACTERISTICS_NO_ISOLATION           0x0200
#define IMAGE_DLLCHARACTERISTICS_NO_SEH                 0x0400
#define IMAGE_DLLCHARACTERISTICS_NO_BIND                0x0800
#define IMAGE_DLLCHARACTERISTICS_WDM_DRIVER             0x2000
#define IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE  0x8000

/* they actually defined 0x00000000 as well, but I think we'll skip that one. */
#define IMAGE_SCN_RESERVED_0	0x00000001
#define IMAGE_SCN_RESERVED_1	0x00000002
#define IMAGE_SCN_RESERVED_2	0x00000004
#define IMAGE_SCN_TYPE_NO_PAD	0x00000008 /* don't pad - obsolete */
#define IMAGE_SCN_RESERVED_3	0x00000010
#define IMAGE_SCN_CNT_CODE	0x00000020 /* .text */
#define IMAGE_SCN_CNT_INITIALIZED_DATA 0x00000040 /* .data */
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x00000080 /* .bss */
#define IMAGE_SCN_LNK_OTHER	0x00000100 /* reserved */
#define IMAGE_SCN_LNK_INFO	0x00000200 /* .drectve comments */
#define IMAGE_SCN_RESERVED_4	0x00000400
#define IMAGE_SCN_LNK_REMOVE	0x00000800 /* .o only - scn to be rm'd*/
#define IMAGE_SCN_LNK_COMDAT	0x00001000 /* .o only - COMDAT data */
#define IMAGE_SCN_RESERVED_5	0x00002000 /* spec omits this */
#define IMAGE_SCN_RESERVED_6	0x00004000 /* spec omits this */
#define IMAGE_SCN_GPREL		0x00008000 /* global pointer referenced data */
/* spec lists 0x20000 twice, I suspect they meant 0x10000 for one of them */
#define IMAGE_SCN_MEM_PURGEABLE	0x00010000 /* reserved for "future" use */
#define IMAGE_SCN_16BIT		0x00020000 /* reserved for "future" use */
#define IMAGE_SCN_LOCKED	0x00040000 /* reserved for "future" use */
#define IMAGE_SCN_PRELOAD	0x00080000 /* reserved for "future" use */
/* and here they just stuck a 1-byte integer in the middle of a bitfield */
#define IMAGE_SCN_ALIGN_1BYTES	0x00100000 /* it does what it says on the box */
#define IMAGE_SCN_ALIGN_2BYTES	0x00200000
#define IMAGE_SCN_ALIGN_4BYTES	0x00300000
#define IMAGE_SCN_ALIGN_8BYTES	0x00400000
#define IMAGE_SCN_ALIGN_16BYTES	0x00500000
#define IMAGE_SCN_ALIGN_32BYTES	0x00600000
#define IMAGE_SCN_ALIGN_64BYTES	0x00700000
#define IMAGE_SCN_ALIGN_128BYTES 0x00800000
#define IMAGE_SCN_ALIGN_256BYTES 0x00900000
#define IMAGE_SCN_ALIGN_512BYTES 0x00a00000
#define IMAGE_SCN_ALIGN_1024BYTES 0x00b00000
#define IMAGE_SCN_ALIGN_2048BYTES 0x00c00000
#define IMAGE_SCN_ALIGN_4096BYTES 0x00d00000
#define IMAGE_SCN_ALIGN_8192BYTES 0x00e00000
#define IMAGE_SCN_LNK_NRELOC_OVFL 0x01000000 /* extended relocations */
#define IMAGE_SCN_MEM_DISCARDABLE 0x02000000 /* scn can be discarded */
#define IMAGE_SCN_MEM_NOT_CACHED 0x04000000 /* cannot be cached */
#define IMAGE_SCN_MEM_NOT_PAGED	0x08000000 /* not pageable */
#define IMAGE_SCN_MEM_SHARED	0x10000000 /* can be shared */
#define IMAGE_SCN_MEM_EXECUTE	0x20000000 /* can be executed as code */
#define IMAGE_SCN_MEM_READ	0x40000000 /* readable */
#define IMAGE_SCN_MEM_WRITE	0x80000000 /* writeable */

#define IMAGE_DEBUG_TYPE_CODEVIEW	2

#endif /* __LINUX_PE_H */
