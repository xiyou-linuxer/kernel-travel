/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2009 Chen Liqin <liqin.chen@sunplusct.com>
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2017 SiFive
 * Copyright (C) 2017 XiaojingZhu <zhuxiaoj@ict.ac.cn>
 */

#ifndef _ASM_RISCV_PAGE_H
#define _ASM_RISCV_PAGE_H

#include <xkernel/const.h>
#include <xkernel/types.h>

#define PAGE_SHIFT	12
#define PAGE_SIZE	(_AC(1, UL) << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))

#define PAGE_ALIGN(addr) ALIGN(addr, PAGE_SIZE)

/*
 * PAGE_OFFSET -- the first address of the first page of memory.
 * When not using MMU this corresponds to the first free page in
 * physical memory (aligned on a page boundary).
 */
#ifdef CONFIG_64BIT
#ifdef CONFIG_MMU
#define PAGE_OFFSET		kernel_map.page_offset
#else
#define PAGE_OFFSET		_AC(CONFIG_PAGE_OFFSET, UL)
#endif
/*
 * By default, CONFIG_PAGE_OFFSET value corresponds to SV57 address space so
 * define the PAGE_OFFSET value for SV48 and SV39.
 */
#define PAGE_OFFSET_L4		_AC(0xffffaf8000000000, UL)
#define PAGE_OFFSET_L3		_AC(0xffffffd800000000, UL)
#else
#define PAGE_OFFSET		_AC(CONFIG_PAGE_OFFSET, UL)
#endif /* CONFIG_64BIT */

#ifndef __ASSEMBLY__

#ifdef CONFIG_RISCV_ISA_ZICBOZ
void clear_page(void *page);
#else
#define clear_page(pgaddr)			memset((pgaddr), 0, PAGE_SIZE)
#endif
#define copy_page(to, from)			memcpy((to), (from), PAGE_SIZE)

#define clear_user_page(pgaddr, vaddr, page)	clear_page(pgaddr)
#define copy_user_page(vto, vfrom, vaddr, topg) \
			memcpy((vto), (vfrom), PAGE_SIZE)

/*
 * Use struct definitions to apply C type checking
 */

/* Page Global Directory entry */
typedef struct {
	unsigned long pgd;
} pgd_t;

/* Page Table entry */
typedef struct {
	unsigned long pte;
} pte_t;

typedef struct {
	unsigned long pgprot;
} pgprot_t;

typedef struct page *pgtable_t;

#define pte_val(x)	((x).pte)
#define pgd_val(x)	((x).pgd)
#define pgprot_val(x)	((x).pgprot)

#define __pte(x)	((pte_t) { (x) })
#define __pgd(x)	((pgd_t) { (x) })
#define __pgprot(x)	((pgprot_t) { (x) })

#ifdef CONFIG_64BIT
#define PTE_FMT "%016lx"
#else
#define PTE_FMT "%08lx"
#endif

#endif /* _ASM_RISCV_PAGE_H */

#endif
