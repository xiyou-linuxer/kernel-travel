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
#include <asm/asm.h>
#include <xkernel/string.h>
#include <xkernel/debug.h>
#ifndef __maybe_unused
# define __maybe_unused		__attribute__((unused))
#endif

void *_dtb_early_va __initdata;
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

static pmd_t trampoline_pmd[PTRS_PER_PMD] __page_aligned_bss;
static pmd_t fixmap_pmd[PTRS_PER_PMD] __page_aligned_bss;
static pmd_t early_pmd[PTRS_PER_PMD] __initdata __aligned(PAGE_SIZE);

#define fixmap_pgd_next		((uintptr_t)fixmap_pmd)

#define pgd_next_t		p4d_t

#define alloc_pgd_next(__va)	pt_ops.alloc_pmd(__va)
/* 获得下一个页表项 */
#define get_pgd_next_virt(__pa)	(pud_t *)pt_ops.get_pmd_virt(__pa)

#else 	/*__PAGETABLE_PMD_FOLDED*/
#define pgd_next_t		pte_t
#define alloc_pgd_next(__va)	pt_ops.alloc_pte(__va)

#define get_pgd_next_virt(__pa)	pt_ops.get_pte_virt(__pa)

#endif	/*__PAGETABLE_PMD_FOLDED*/

#ifndef pgd_index
/* Must be a compile-time constant, so implement it as a macro */
#define pgd_index(a)  (((a) >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1))

#endif/*pgd_index*/
#ifndef __PAGETABLE_PMD_FOLDED
	phys_addr_t virt_to_phys(void *vaddr){

	unsigned long addr = (unsigned long)vaddr;
    unsigned long pgd_index = (addr >> PGDIR_SHIFT) & 0x1FF; // 高 9 位
    unsigned long pmd_index = (addr >> PMD_SHIFT) & 0x1FF; // 中 9 位
    unsigned long pte_index = (addr >> PFN_PTE_SHIFT) & 0x1FF; // 低 9 位
    unsigned long offset = addr & (PAGE_SIZE - 1); // 页内偏移

    // 获取PGD、PMD、PTE
    pgd_t *pgd = &early_pg_dir[pgd_index];
    pmd_t *pmd = &early_pmd[(addr >> 21) & (512 -1)];
    pte_t *pte = pt_ops.alloc_pte(addr); //[(addr >> 12) & (512 - 1)] ;  

    // 获取物理页框号（PFN）
    unsigned long pfn = pte_pfn(*pte) >> PAGE_SHIFT; // 页表项指向物理页框号

    // 计算物理地址
    unsigned long phys_addr = (pfn << PAGE_SHIFT) | offset;
    
    return phys_addr;
	}


 void __init create_pmd_mapping(pmd_t *pmdp,
				      uintptr_t va, phys_addr_t pa,
				      phys_addr_t sz, pgprot_t prot){
	pte_t *ptep;
	phys_addr_t pte_phys;
	uintptr_t pmd_idx = pmd_index(va);

	if (sz == PMD_SIZE) {
		if (pmd_val(pmdp[pmd_idx])==0)
			pmdp[pmd_idx] = pfn_pmd(PFN_DOWN(pa), prot);
		return;
	}

	if (pmd_val(pmdp[pmd_idx])==0){
		pte_phys = pt_ops.alloc_pte(va);
		pmdp[pmd_idx] = pfn_pmd(PFN_DOWN(pte_phys), PAGE_TABLE);
		ptep = pt_ops.get_pte_virt(pte_phys);
		memset(ptep, 0, PAGE_SIZE);
	} else {
		pte_phys = PFN_PHYS(_pmd_pfn(pmdp[pmd_idx]));
		ptep = pt_ops.get_pte_virt(pte_phys);
	}

	create_pte_mapping(ptep, va, pa, sz, prot);
}


#define create_pgd_next_mapping(__nextp, __va, __pa, __sz, __prot)	create_pmd_mapping((pmd_t *)__nextp, __va, __pa, __sz, __prot)


static pmd_t *__init get_pmd_virt_early(phys_addr_t pa)
{
	/* Before MMU is enabled */
	return (pmd_t *)((uintptr_t)pa);
}

static phys_addr_t __init alloc_pmd_early(uintptr_t va)
{
	// BUG_ON((va - kernel_map.virt_addr) >> PUD_SHIFT);

	return (uintptr_t)early_pmd;
}

#else

#define create_p4d_mapping(__pmdp, __va, __pa, __sz, __prot) do {} while(0)
#define create_pud_mapping(__pmdp, __va, __pa, __sz, __prot) do {} while(0)
#define create_pmd_mapping(__pmdp, __va, __pa, __sz, __prot) do {} while(0)


#endif


static inline phys_addr_t __init alloc_pte_early(uintptr_t va)
{
	/*
	 * We only create PMD or PGD early mappings so we
	 * should never reach here with MMU disabled.
	 * 
	 * 我们只创建 PMD 或 PGD 早期映射，因此
	 * 在 MMU 禁用的情况下，我们永远不会到达这里。
	 */
	// BUG();

}

static inline pte_t *__init get_pte_virt_early(phys_addr_t pa)
{
	return (pte_t *)((uintptr_t)pa);
}


static void __init pt_ops_set_early(void){
	pt_ops.alloc_pte = alloc_pte_early;
	pt_ops.get_pte_virt = get_pte_virt_early;
	#ifndef __PAGETABLE_PMD_FOLDED
	pt_ops.alloc_pmd = alloc_pmd_early;
	pt_ops.get_pmd_virt = get_pmd_virt_early;
	#endif
}




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
	//  映射内存区等与一个顶级页表项（2<<（39））
	if (sz == PGDIR_SIZE) {
		//如果是在PGD页表为空
		if (pgd_val(pgdp[pgd_idx]) == 0)
			pgdp[pgd_idx] = pfn_pgd(PFN_DOWN(pa), prot); //右移12,加上PAGE_KERNEL 0000000000111111 
			printk("pgd_val(pgdp[pgd_idx])=%lx \nPFN_DOWN(pa)= %lx \nprot= %lx  pgd_idx=  %d\n",pgd_val(pgdp[pgd_idx]),PFN_DOWN(pa),prot,pgd_idx);
		return;
	}
	printk("\n\n\n\n\n");
	printk("pgd_val(pgdp[pgd_idx])=%d \n pgd_idx=  %d\n",pgd_val(pgdp[pgd_idx]),pgd_idx);

	
	if (pgd_val(pgdp[pgd_idx]) == 0) {
		printk("create_pgd_mapping1\n");
		next_phys = alloc_pgd_next(va);
		pgdp[pgd_idx] = pfn_pgd(PFN_DOWN(next_phys), PAGE_TABLE);
		nextp = get_pgd_next_virt(next_phys);
		memset(nextp, 0, PAGE_SIZE);
	} else {
		next_phys = PFN_PHYS(_pgd_pfn(pgdp[pgd_idx]));
		nextp = get_pgd_next_virt(next_phys);
	}
	create_pgd_next_mapping(nextp, va, pa, sz, prot);
	
	//刷新快表
	// local_flush_tlb_all();

}


void __init create_pte_mapping(pte_t *ptep,
				      uintptr_t va, phys_addr_t pa,
				      phys_addr_t sz, pgprot_t prot)
{
	uintptr_t pte_idx = pte_index(va);

	// BUG_ON(sz != PAGE_SIZE);

	if (pte_val(ptep[pte_idx])==0)
		ptep[pte_idx] = pfn_pte(PFN_DOWN(pa), prot);
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
	//__pi_set_satp_mode_from_cmdline(dtb_pa);
	//
	// __pi_set_satp_mode_from_cmdline(dtb_pa);
	if (satp_mode_cmdline == SATP_MODE_57) {
		//disable_pgtable_l5();
			printk("\n6");
	} else if (satp_mode_cmdline == SATP_MODE_48) {
		// disable_pgtable_l5();
		// disable_pgtable_l4();
		return;
	} /*默认Sv39 */
	// create_p4d_mapping(early_p4d,
	// 		set_satp_mode_pmd, (uintptr_t)early_pud,
	// 		P4D_SIZE, PAGE_TABLE);
	// create_pud_mapping(early_pud,
	// 		   set_satp_mode_pmd, (uintptr_t)early_pmd,
	// 		   PUD_SIZE, PAGE_TABLE);
	// /* 处理 set_satp_mode 跨越 2 个 PMD 的情况 */
	// create_pmd_mapping(early_pmd,
	// 		   set_satp_mode_pmd, set_satp_mode_pmd,
	// 		   PMD_SIZE, PAGE_KERNEL_EXEC);
	// create_pmd_mapping(early_pmd,
	// 		   set_satp_mode_pmd + PMD_SIZE,
	// 		   set_satp_mode_pmd + PMD_SIZE,
	// 		   PMD_SIZE, PAGE_KERNEL_EXEC);	
// retry:
/* 未在这里验证页表*/

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
	printk("XIP Kernel Mode\n");
	#else
	// kernel_map.page_offset = _AC(CONFIG_PAGE_OFFSET, UL);
	// kernel_map.phys_addr = (uintptr_t)(&_start);
	// kernel_map.size = (uintptr_t)(&_end) - kernel_map.phys_addr;
	#endif /*CONFIG_XIP_KERNEL*/
	printk("Setting up Virtual Memory\n");	
	
	#if defined(CONFIG_64BIT) && !defined(CONFIG_XIP_KERNEL)
	set_satp_mode(dtb_pa);
	set_mmap_rnd_bits_max();
	printk("mmap_rnd_bits_max==%d\n",mmap_rnd_bits_max); //为24的话运行在Sv39模式下
	#endif

	pt_ops_set_early();
	printk("1234578875643\n");
	create_pgd_mapping(early_pg_dir,(uintptr_t)0XFFFFFFFF80000000, (uintptr_t)0X0000000080000000, PGDIR_SIZE, PAGE_KERNEL);
// (phys_addr_t)fixmap_pgd_next

	uintptr_t virt_addr = 0xFFFFFFFF80000000;
    uintptr_t phys_addr = 0x0000000080000000;

    // 读取虚拟地址对应的物理地址
    uintptr_t mapped_addr = (uintptr_t)virt_to_phys((void *)virt_addr);

    if (mapped_addr == phys_addr) {
        printk("Mapping successful: Virtual address 0x%lx maps to Physical address 0x%lx\n", virt_addr, phys_addr);
    } else {
        printk("Mapping failed: Virtual address 0x%lx maps to Physical address 0x%lx\n", virt_addr, mapped_addr);
    }
	
 	printk("VM setup completed\n");
}
#else

asmlinkage void __init setup_vm(uintptr_t dtb_pa)
{

	dtb_early_va = (void *)dtb_pa;
	dtb_early_pa = dtb_pa;
	
	printk("111111");	
}

#endif