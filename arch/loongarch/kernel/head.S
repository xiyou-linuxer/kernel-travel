#include <linux/init.h>
#include <linux/linkage.h>
#include <linux/pe.h>
#include <linux/size.h>

#include <asm/loongarch.h>
#include <asm/regdef.h>
#include <asm/asm.h>
#include <asm/thread_info.h>
#include <asm/stackframe.h>

#ifndef CONFIG_2K1000LA
	.macro	__EFI_PE_HEADER
	.long	PE_MAGIC
.Lcoff_header:
	.short	IMAGE_FILE_MACHINE_LOONGARCH64		/* Machine */
	.short	.Lsection_count				/* NumberOfSections */
	.long	0 					/* TimeDateStamp */
	.long	0					/* PointerToSymbolTable */
	.long	0					/* NumberOfSymbols */
	.short	.Lsection_table - .Loptional_header	/* SizeOfOptionalHeader */
	.short	IMAGE_FILE_DEBUG_STRIPPED | \
		IMAGE_FILE_EXECUTABLE_IMAGE | \
		IMAGE_FILE_LINE_NUMS_STRIPPED		/* Characteristics */

.Loptional_header:
	.short	PE_OPT_MAGIC_PE32PLUS			/* PE32+ format */
	.byte	0x02					/* MajorLinkerVersion */
	.byte	0x14					/* MinorLinkerVersion */
	.long	__inittext_end - .Lefi_header_end	/* SizeOfCode */
	.long	_end - __initdata_begin			/* SizeOfInitializedData */
	.long	0					/* SizeOfUninitializedData */
	.long	efi_pe_entry - _head			/* AddressOfEntryPoint */
	.long	.Lefi_header_end - _head		/* BaseOfCode */

.Lextra_header_fields:
	.quad	0					/* ImageBase */
	.long	PECOFF_SEGMENT_ALIGN			/* SectionAlignment */
	.long	PECOFF_FILE_ALIGN			/* FileAlignment */
	.short	0					/* MajorOperatingSystemVersion */
	.short	0					/* MinorOperatingSystemVersion */
	.short	LINUX_EFISTUB_MAJOR_VERSION		/* MajorImageVersion */
	.short	LINUX_EFISTUB_MINOR_VERSION		/* MinorImageVersion */
	.short	0					/* MajorSubsystemVersion */
	.short	0					/* MinorSubsystemVersion */
	.long	0					/* Win32VersionValue */

	.long	_end - _head				/* SizeOfImage */

	/* Everything before the kernel image is considered part of the header */
	.long	.Lefi_header_end - _head		/* SizeOfHeaders */
	.long	0					/* CheckSum */
	.short	IMAGE_SUBSYSTEM_EFI_APPLICATION		/* Subsystem */
	.short	0					/* DllCharacteristics */
	.quad	0					/* SizeOfStackReserve */
	.quad	0					/* SizeOfStackCommit */
	.quad	0					/* SizeOfHeapReserve */
	.quad	0					/* SizeOfHeapCommit */
	.long	0					/* LoaderFlags */
	.long	(.Lsection_table - .) / 8		/* NumberOfRvaAndSizes */

	.quad	0					/* ExportTable */
	.quad	0					/* ImportTable */
	.quad	0					/* ResourceTable */
	.quad	0					/* ExceptionTable */
	.quad	0					/* CertificationTable */
	.quad	0					/* BaseRelocationTable */

	/* Section table */
.Lsection_table:
	.ascii	".text\0\0\0"
	.long	__inittext_end - .Lefi_header_end	/* VirtualSize */
	.long	.Lefi_header_end - _head		/* VirtualAddress */
	.long	__inittext_end - .Lefi_header_end	/* SizeOfRawData */
	.long	.Lefi_header_end - _head		/* PointerToRawData */

	.long	0					/* PointerToRelocations */
	.long	0					/* PointerToLineNumbers */
	.short	0					/* NumberOfRelocations */
	.short	0					/* NumberOfLineNumbers */
	.long	IMAGE_SCN_CNT_CODE | \
		IMAGE_SCN_MEM_READ | \
		IMAGE_SCN_MEM_EXECUTE			/* Characteristics */

	.ascii	".data\0\0\0"
	.long	_end - __initdata_begin			/* VirtualSize */
	.long	__initdata_begin - _head		/* VirtualAddress */
	.long	_edata - __initdata_begin		/* SizeOfRawData */
	.long	__initdata_begin - _head		/* PointerToRawData */

	.long	0					/* PointerToRelocations */
	.long	0					/* PointerToLineNumbers */
	.short	0					/* NumberOfRelocations */
	.short	0					/* NumberOfLineNumbers */
	.long	IMAGE_SCN_CNT_INITIALIZED_DATA | \
		IMAGE_SCN_MEM_READ | \
		IMAGE_SCN_MEM_WRITE			/* Characteristics */

	.set	.Lsection_count, (. - .Lsection_table) / 40

	.balign	0x10000					/* PECOFF_SEGMENT_ALIGN */
.Lefi_header_end:
	.endm

	__HEAD
	
_head:
	.word MZ_MAGIC
	.org	0x8
	.dword kernel_entry
	.dword	_end - _text		/* Kernel image effective size */
	.quad	0			/* Kernel image load offset from start of RAM */
	.org	0x38			/* 0x20 ~ 0x37 reserved */
	.long	LINUX_PE_MAGIC
	.long	pe_header - _head	/* Offset to the PE header */

pe_header:
	__EFI_PE_HEADER

#endif

	.section ".head.text","ax"

	.align 12

SYM_CODE_START(kernel_entry)			# kernel entry point

	/** 
	 * 配置直接映射窗口
	 * 设置CRMD.PG位
	 */
	li.d		t0, CSR_DMW0_INIT	# UC, PLV0, 0x8000 xxxx xxxx xxxx
	csrwr		t0, LOONGARCH_CSR_DMWIN0
	li.d		t0, CSR_DMW1_INIT	# 一致可缓存（Coherent Cached，CA）存储访问类型，PLV0下可以使用该窗口的配置进行直接映射地址翻译，地址窗口为0x9000 xxxx xxxx xxxx
	csrwr		t0, LOONGARCH_CSR_DMWIN1

	// JUMP_VIRT_ADDR	t0, t1

	/* Enable PG */
	li.w		t0, 0xb0		# 1011 0000, PLV=0, IE=0, PG=1, DATF=1
	csrwr		t0, LOONGARCH_CSR_CRMD
	li.w		t0, 0x04		# 0000 0100, PLV=0, PIE=1, PWE=0
	csrwr		t0, LOONGARCH_CSR_PRMD
	li.w		t0, 0x00		# 0000 0000, FPE=0, SXE=0, ASXE=0, BTE=0, 不使用扩展
	csrwr		t0, LOONGARCH_CSR_EUEN

	/* We might not get launched at the address the kernel is linked to,
	   so we jump there.  */
	la.abs		t0, 0f
	jirl		zero, t0, 0
0:

	la.pcrel	t0, __bss_start		# 清空.bss段
	st.d		zero, t0, 0
	la.pcrel	t1, __bss_stop - LONGSIZE
1:
	addi.d		t0, t0, LONGSIZE
	st.d		zero, t0, 0
	bne		t0, t1, 1b

	/**
	 * 保存固件传递的参数
	 */
	la.pcrel	t0, fw_arg0
	st.d		a0, t0, 0
	la.pcrel	t0, fw_arg1
	st.d		a1, t0, 0
	la.pcrel	t0, fw_arg2
	st.d		a2, t0, 0

	/* KSave3 used for percpu base, initialized as 0 */
	csrwr		zero, PERCPU_BASE_KS
	/* GPR21 used for percpu base (runtime), initialized as 0 */
	move		u0, zero

	la.pcrel	tp, init_thread_union
	/* Set the SP after an empty pt_regs.  */
	li.d		sp, (0x00004000 - PT_SIZE)
	add.d		sp, sp, tp
	/* set_saved_sp	sp, t0, t1 */
	la.abs		t0, kernelsp
	st.d		sp, t0, 0

	bl		start_kernel
	/* ASM_BUG() */

SYM_CODE_END(kernel_entry)

SYM_ENTRY(kernel_entry_end, SYM_L_GLOBAL, SYM_A_NONE)

SYM_DATA(kernel_asize, .long _end - _text);
SYM_DATA(kernel_fsize, .long _edata - _text);
SYM_DATA(kernel_offset, .long kernel_offset - _text);
