#include <xkernel/init.h>
#include <xkernel/linkage.h>
#include <xkernel/types.h>
#include <xkernel/compiler.h>
#include <xkernel/stdio.h>
#include <xkernel/memory.h>
#include <xkernel/mm.h>
#include <asm/cache.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/csr.h>
#include <xkernel/string.h>
#ifndef __maybe_unused
# define __maybe_unused		__attribute__((unused))
#endif


void *dtb_early_va;
static phys_addr_t dtb_early_pa __initdata;
struct kernel_mapping kernel_map ;
/* 每个页全局目录（PGD）包含的指针数量 */
#define PTRS_PER_PGD    (PAGE_SIZE / sizeof(pgd_t))  

/* 页全局目录*/
pgd_t early_pg_dir[PTRS_PER_PGD] __initdata __aligned(PAGE_SIZE);



#ifdef CONFIG_MMU

/* 操作函数结构体*/
struct pt_alloc_ops pt_ops __initdata;

#ifndef __PAGETABLE_PMD_FOLDED
#define pgd_next_t		p4d_t

#define alloc_pgd_next(__va)	(pgtable_l5_enabled ?			\
pt_ops.alloc_p4d(__va) : (pgtable_l4_enabled ?		\
pt_ops.alloc_pud(__va) : pt_ops.alloc_pmd(__va)))

#else 	/*__PAGETABLE_PMD_FOLDED*/
#define pgd_next_t		pte_t
#define alloc_pgd_next(__va)	pt_ops.alloc_pte(__va)
#endif	/*__PAGETABLE_PMD_FOLDED*/

#ifndef pgd_index
/* Must be a compile-time constant, so implement it as a macro */
#define pgd_index(a)  (((a) >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1))

#endif/*pgd_index*/

#define create_p4d_mapping(__pmdp, __va, __pa, __sz, __prot) do {} while(0)
#define create_pud_mapping(__pmdp, __va, __pa, __sz, __prot) do {} while(0)
#define create_pmd_mapping(__pmdp, __va, __pa, __sz, __prot) do {} while(0)


/**
 * create_pgd_mapping - 创建一个页全局目录（PGD）映射
 * @pgdp: 目标 PGD 表指针
 * @va: 虚拟地址的起始位置
 * @pa: 物理地址的起始位置
 * @sz: 映射的大小（字节）
 * @prot: 页表项的权限（如读/写/执行）
 * 
 * 此函数创建一个虚拟地址到物理地址的映射，并更新相应的 PGD 表项。
 * 该映射用于将一段物理内存（pa）映射到一段虚拟内存（va），并设置页表项的权限（prot）。
 * 
 * 在初始化阶段使用此函数来建立内核的地址空间映射。
 * 
 * 该函数假定传入的 va、pa 和 sz 参数是按页对齐的。
 * 
 * 返回值: 无
 */
void __init create_pgd_mapping(pgd_t *pgdp,
				      uintptr_t va, phys_addr_t pa,
				      phys_addr_t sz, pgprot_t prot)
{
	pgd_next_t *nextp;
	phys_addr_t next_phys;	
	uintptr_t pgd_idx = pgd_index(va);
	// 优化—— 映射内存区等与一个顶级页表项（2<<（39））
	if (sz == PGDIR_SIZE) {
		//如果是在PGD页表为空
		if (pgd_val(pgdp[pgd_idx]) == 0)
			pgdp[pgd_idx] = pfn_pgd(PFN_DOWN(pa), prot);
		return;
	}
	// 
	// if (pgd_val(pgdp[pgd_idx]) == 0) {
	// 	next_phys = alloc_pgd_next(va);
	// 	pgdp[pgd_idx] = pfn_pgd(PFN_DOWN(next_phys), PAGE_TABLE);
	// 	nextp = get_pgd_next_virt(next_phys);
	// 	memset(nextp, 0, PAGE_SIZE);
	// } else {
	// 	next_phys = PFN_PHYS(_pgd_pfn(pgdp[pgd_idx]));
	// 	nextp = get_pgd_next_virt(next_phys);
	// }
	// create_pgd_next_mapping(nextp, va, pa, sz, prot);
}


static pmd_t early_pmd[PTRS_PER_PMD] __initdata __aligned(PAGE_SIZE);

#ifdef CONFIG_XIP_KERNEL
#define early_pmd      ((pmd_t *)XIP_FIXUP(early_pmd))
#endif	/*CONFIG_XIP_KERNEL*/


#if defined(CONFIG_64BIT) && !defined(CONFIG_XIP_KERNEL)
u64 __pi_set_satp_mode_from_cmdline(uintptr_t dtb_pa);
#endif
/*
* 有一种简单的方法可以确定底层硬件是否支持 4 级页表：在 4 级页表模式下建立 1:1 映射
* 然后读取 SATP(寄存器) 以查看配置是否被考虑在内表示支持 sv48
*/
static __init void set_satp_mode(uintptr_t dtb_pa){
	u64 identity_satp, hw_satp;

	uintptr_t set_satp_mode_pmd = ((unsigned long)set_satp_mode) & PMD_MASK;
	u64 satp_mode_cmdline = SATP_MODE_39;
	// __pi_set_satp_mode_from_cmdline(dtb_pa);
	// if (satp_mode_cmdline == SATP_MODE_57) {
	// 	disable_pgtable_l5();
	// } else if (satp_mode_cmdline == SATP_MODE_48) {
	// 	disable_pgtable_l5();
	// 	disable_pgtable_l4();
	// 	return;
	// } /*默认Sv39 */
	create_p4d_mapping(early_p4d,
			set_satp_mode_pmd, (uintptr_t)early_pud,
			P4D_SIZE, PAGE_TABLE);
	create_pud_mapping(early_pud,
			   set_satp_mode_pmd, (uintptr_t)early_pmd,
			   PUD_SIZE, PAGE_TABLE);
	/* 处理 set_satp_mode 跨越 2 个 PMD 的情况 */
	create_pmd_mapping(early_pmd,
			   set_satp_mode_pmd, set_satp_mode_pmd,
			   PMD_SIZE, PAGE_KERNEL_EXEC);
	create_pmd_mapping(early_pmd,
			   set_satp_mode_pmd + PMD_SIZE,
			   set_satp_mode_pmd + PMD_SIZE,
			   PMD_SIZE, PAGE_KERNEL_EXEC);	
// retry:
/* 未完成*/

}

static void __init set_mmap_rnd_bits_max(void)
{
	mmap_rnd_bits_max = MMAP_VA_BITS - PAGE_SHIFT - 3;// 
}



asmlinkage void __init setup_vm(uintptr_t dtb_pa){
	/* 页中间目录*/
	pmd_t __maybe_unused fix_bmap_spmd, fix_bmap_epmd;
	#ifdef CONFIG_RANDOMIZE_BASE
	
	#endif
	kernel_map.virt_addr = 	 + kernel_map.virt_offset;

	#ifdef CONFIG_XIP_KERNEL
	printk("22222\n");
	#else
	// kernel_map.page_offset = _AC(CONFIG_PAGE_OFFSET, UL);
	// kernel_map.phys_addr = (uintptr_t)(&_start);
	// kernel_map.size = (uintptr_t)(&_end) - kernel_map.phys_addr;
	#endif /*CONFIG_XIP_KERNEL*/
	printk("22222\n");	
	
	#if defined(CONFIG_64BIT) && !defined(CONFIG_XIP_KERNEL)
	set_satp_mode(dtb_pa);
	set_mmap_rnd_bits_max();
	printk("mmap_rnd_bits_max==%d\n",mmap_rnd_bits_max); //为24的话运行在Sv39模式下
	
#endif


}
#else

asmlinkage void __init setup_vm(uintptr_t dtb_pa)
{

	dtb_early_va = (void *)dtb_pa;
	dtb_early_pa = dtb_pa;
	
	printk("111111");	
}

#endif