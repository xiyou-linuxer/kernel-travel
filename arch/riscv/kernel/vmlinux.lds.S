/* SPDX-License-Identifier: GPL-2.0 */
#include <xkernel/sizes.h>
#include <asm/asm-offsets.h>
#include <asm/thread_info.h>

#define RO_EXCEPTION_TABLE_ALIGN	4

/*
 * Put .bss..swapper_pg_dir as the first thing in .bss. This will
 * ensure that it has .bss alignment (64K).
 */
#define BSS_FIRST_SECTIONS *(.bss..swapper_pg_dir)

#include <asm-generic/vmlinux.lds.h>
#include "image-vars.h"

PECOFF_SEGMENT_ALIGN = 0x10000;

OUTPUT_ARCH(riscv)
ENTRY(kernel_entry)
PHDRS {
	text PT_LOAD FLAGS(7);	/* RWX */
	note PT_NOTE FLAGS(4);	/* R__ */
}

SECTIONS
{
	. = 0xffffffff80000000;

	_text = .;
	HEAD_TEXT_SECTION

	. = ALIGN(PECOFF_SEGMENT_ALIGN);
	_stext = .;
	.text : {
		TEXT_TEXT
		SCHED_TEXT
		CPUIDLE_TEXT
		LOCK_TEXT
		KPROBES_TEXT
		IRQENTRY_TEXT
		SOFTIRQENTRY_TEXT
		*(.fixup)
		*(.gnu.warning)
	} :text = 0
	. = ALIGN(PECOFF_SEGMENT_ALIGN);
	_etext = .;

	/* Alternative instruction section */
	. = ALIGN(4);
	.altinstructions : AT(ADDR(.altinstructions) - LOAD_OFFSET) {
		__alt_instructions = .;
		*(.altinstructions)
		__alt_instructions_end = .;
	}

	/* RISC-V specific sections */
	.plt : { *(.plt) }
	.got : { *(.got) }
	.got.plt : { *(.got.plt) }

	. = ALIGN(PECOFF_SEGMENT_ALIGN);
	__init_begin = .;
	__inittext_begin = .;

	INIT_TEXT_SECTION(PAGE_SIZE)
	.exit.text : {
		EXIT_TEXT
	}

	. = ALIGN(PECOFF_SEGMENT_ALIGN);
	__inittext_end = .;

	__initdata_begin = .;

	INIT_DATA_SECTION(16)
	.exit.data : {
		EXIT_DATA
	}

#ifdef CONFIG_SMP
	PERCPU_SECTION(1 << CONFIG_L1_CACHE_SHIFT)
#endif

	.rela.dyn : ALIGN(8) { *(.rela.dyn) *(.rela*) }

	.init.bss : {
		*(.init.bss)
	}
	. = ALIGN(PECOFF_SEGMENT_ALIGN);
	__initdata_end = .;

	__init_end = .;

	_sdata = .;
	RO_DATA(PAGE_SIZE)
	RW_DATA(1 << CONFIG_L1_CACHE_SHIFT, PAGE_SIZE, THREAD_SIZE)

	.sdata : {
		__global_pointer$ = . + 0x800;
		*(.sdata*)
		*(.sdata2*)
	}
	_edata = .;

	.bss : {
    	__bss_start = .;
    	*(.bss)
    	*(COMMON)
    	__bss_stop = .;
	}
	
	. = ALIGN(PECOFF_SEGMENT_ALIGN);

	_end = .;

	/* Debugging sections */
	STABS_DEBUG
	DWARF_DEBUG
	ELF_DETAILS

	/* Discard unnecessary sections */
	DISCARDS
	/DISCARD/ : {
		*(.gnu.attributes)
		*(.options)
		*(.eh_frame)
	}
}