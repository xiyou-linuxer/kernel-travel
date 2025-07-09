#ifndef _ASM_RISCV_PAGEABLE_H
#define _ASM_RISCV_PAGEABLE_H

extern void *dtb_early_va;

/* */

#include <xkernel/size.h>
#include <xkernel/mm.h>
#include <asm/pgtable-bits.h>
#include <asm/page.h>
#ifndef CONFIG_MMU
#define KERNEL_LINK_ADDR	PAGE_OFFSET
#define KERN_VIRT_SIZE		(UL(-1))
#else /*CONFIG_MMU*/
#define ADDRESS_SPACE_END	(UL(-1))
#define KERNEL_LINK_ADDR	(ADDRESS_SPACE_END - SZ_2G + 1)
#endif /*CONFIG_MMU*/


#ifndef __maybe_unused
# define __maybe_unused		__attribute__((unused))
#endif
static const __maybe_unused int pgtable_l4_enabled;
static const __maybe_unused int pgtable_l5_enabled;

#define PGDIR_SHIFT_L3  30
#define PGDIR_SHIFT_L4  39
#define PGDIR_SHIFT_L5  48
#define PGDIR_SHIFT   30 // 使用Sv39

typedef struct {
	unsigned long pmd;
} pmd_t;

#define pmd_val(x)      ((x).pmd)
#define __pmd(x)        ((pmd_t) { (x) })

#define PTRS_PER_PMD    (PAGE_SIZE / sizeof(pmd_t))

typedef struct {
	unsigned long p4d;
} p4d_t;

#define p4d_val(x)	((x).p4d)
#define __p4d(x)	((p4d_t) { (x) })
#define PTRS_PER_P4D	(PAGE_SIZE / sizeof(p4d_t))
typedef struct {
	unsigned long pud;
} pud_t;

#define pud_val(x)      ((x).pud)
#define __pud(x)        ((pud_t) { (x) })
#define PTRS_PER_PUD    (PAGE_SIZE / sizeof(pud_t))


#define PGDIR_SIZE      (_AC(1, UL) << PGDIR_SHIFT)
#define VA_BITS_SV32 32
#ifdef CONFIG_64BIT
#define VA_BITS_SV39 39
#define VA_BITS_SV48 48
#define VA_BITS_SV57 57
#define VA_BITS		(pgtable_l5_enabled ? \
				VA_BITS_SV57 : (pgtable_l4_enabled ? VA_BITS_SV48 : VA_BITS_SV39))
#endif
#define pmd_val(x)      ((x).pmd)
#define __pmd(x)        ((pmd_t) { (x) })
#ifdef CONFIG_MMU

#define _PAGE_KERNEL		(_PAGE_READ \
				| _PAGE_WRITE \
				| _PAGE_PRESENT \
				| _PAGE_ACCESSED \
				| _PAGE_DIRTY \
				| _PAGE_GLOBAL)

#define PAGE_KERNEL		__pgprot(_PAGE_KERNEL)
#define PAGE_KERNEL_READ	__pgprot(_PAGE_KERNEL & ~_PAGE_WRITE)
#define PAGE_KERNEL_EXEC	__pgprot(_PAGE_KERNEL | _PAGE_EXEC)

#endif


/* 每个页中间目录（PMD）包含的指针数量 */

#define PTRS_PER_PMD    (PAGE_SIZE / sizeof(pmd_t))

#define _PAGE_PFN_MASK  GENMASK(53, 10)
/*
 * rv64 PTE format:
 * | 63 | 62 61 | 60 54 | 53  10 | 9             8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
 *   N      MT     RSV    PFN      reserved for SW   D   A   G   U   X   W   R   V
 */
/*
 * [63] Svnapot definitions:
 * 0 Svnapot disabled
 * 1 Svnapot enabled
 */
#define _PAGE_NAPOT_SHIFT	63
#define _PAGE_NAPOT		BIT(_PAGE_NAPOT_SHIFT)


/* 如果是 3 级页表，则将 pud 折叠到 pgd 中 */
#define PUD_SHIFT      30
#define PUD_SIZE       (_AC(1, UL) << PUD_SHIFT)
#define PUD_MASK       (~(PUD_SIZE - 1))

#define PMD_SHIFT       21
/* 页中间目录映射区域的大小 */
#define PMD_SIZE        (_AC(1, UL) << PMD_SHIFT)
#define PMD_MASK        (~(PMD_SIZE - 1))

#ifndef __ASSEMBLY__

#include <asm/page.h>
// #include <asm/tlbflush.h>
// #include <linux/mm_types.h>
// #include <asm/compat.h>

#define MMAP_VA_BITS_64 ((VA_BITS >= VA_BITS_SV48) ? VA_BITS_SV48 : VA_BITS)
#define MMAP_MIN_VA_BITS_64 (VA_BITS_SV39)
// 是否是32位程序运行在64位上
/* 当前CPU架构和内核配置下，所使用的虚拟地址的总位数。*/
#define MMAP_VA_BITS  MMAP_VA_BITS_64

/* 根据物理页帧（PFN）和页表保护，构建PGD条目*/
static inline pgd_t pfn_pgd(unsigned long pfn, pgprot_t prot)
{
	unsigned long prot_val = pgprot_val(prot);

	// ALT_THEAD_PMA(prot_val);

	return __pgd((pfn << _PAGE_PFN_SHIFT) | prot_val); /* pfn物理帧 + prot保护标志位*/
}


struct pt_alloc_ops {
	pte_t *(*get_pte_virt)(phys_addr_t pa);
	phys_addr_t (*alloc_pte)(uintptr_t va);
	#ifndef __PAGETABLE_PMD_FOLDED
	pmd_t *(*get_pmd_virt)(phys_addr_t pa);
	phys_addr_t (*alloc_pmd)(uintptr_t va);
	pud_t *(*get_pud_virt)(phys_addr_t pa);
	phys_addr_t (*alloc_pud)(uintptr_t va);
	p4d_t *(*get_p4d_virt)(phys_addr_t pa);
	phys_addr_t (*alloc_p4d)(uintptr_t va);
#endif
};	
extern struct pt_alloc_ops pt_ops __initdata;
#endif /* __ASSEMBLY__*/

#endif