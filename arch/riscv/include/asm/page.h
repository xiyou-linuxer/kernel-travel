/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2009 Chen Liqin <liqin.chen@sunplusct.com>
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2017 SiFive
 * Copyright (C) 2017 XiaojingZhu <zhuxiaoj@ict.ac.cn>
 */

#ifndef _ASM_RISCV_PAGE_H
#define _ASM_RISCV_PAGE_H


#define PAGE_SHIFT	12
#define PAGE_SIZE	(_AC(1, UL) << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))

#include <xkernel/const.h>
#include <xkernel/types.h>



#ifndef __ASSEMBLY__ 
#define PAGE_ALIGN(addr) ALIGN(addr, PAGE_SIZE)
/* */
struct kernel_mapping {
	unsigned long page_offset; /*内核虚拟地址空间的起始偏移量。*/
	unsigned long virt_addr; /*内核的最终虚拟地址*/
	unsigned long virt_offset; /* KASLR (地址随机化) 计算出的虚拟地址随机偏移量。*/
	uintptr_t phys_addr; /* 内核加载到RAM中的物理起始地址。*/
	uintptr_t size;/*内核在RAM中所占空间的大小。*/
	/* Offset between linear mapping virtual address and kernel load address */
	unsigned long va_pa_offset;/* 线性映射区的虚拟地址与物理内存起始地址之间的偏移量。*/
	/* Offset between kernel mapping virtual address and kernel load address */
	unsigned long va_kernel_pa_offset;/*内核虚拟地址与内核物理地址之间的偏移量。*/
	unsigned long va_kernel_xip_pa_offset;/*(仅XIP模式) 内核虚拟地址与内核在ROM中物理地址的偏移量。*/
#ifdef CONFIG_XIP_KERNEL
	uintptr_t xiprom;
	uintptr_t xiprom_sz;
#endif
};

extern struct kernel_mapping kernel_map;



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

#endif


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


#define PFN_ALIGN(x)	(((unsigned long)(x) + (PAGE_SIZE - 1)) & PAGE_MASK)
#define PFN_UP(x)	(((x) + PAGE_SIZE-1) >> PAGE_SHIFT)
#define PFN_DOWN(x)	((x) >> PAGE_SHIFT)
#define PFN_PHYS(x)	((phys_addr_t)(x) << PAGE_SHIFT)
#define PHYS_PFN(x)	((unsigned long)((x) >> PAGE_SHIFT))




#endif 



#endif/* _ASM_RISCV_PAGE_H */
