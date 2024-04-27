#include <linux/memblock.h>
#include <linux/init.h>
#include <linux/stdio.h>
#include <linux/memory.h>
#include <asm/bootinfo.h>
#include <asm/boot_param.h>
#include <asm/page.h>
#include <bitmap.h>

const struct loongarchlist_mem_map lalist_mem_map = {
    .map_count = 2,  // 内存映射项数量为2
    .map = {
        {
            .mem_type = 0,
            .mem_start = 0x9000000000000000,
            .mem_size = 0x10000000,
        },
        {
            .mem_type = 0,
            .mem_start = 0x9000000090000000,
            .mem_size = 0x30000000,
        }
    }
};


void __init memblock_init(void)
{
	u32 mem_cnt = lalist_mem_map.map_count;
	u64 mem_start, mem_end, mem_size;

	// 物理内存
	mem_start = lalist_mem_map.map[1].mem_start;
	mem_size = lalist_mem_map.map[1].mem_size;
	memblock_add(mem_start,mem_size);
	
	// 保留内存
	mem_start = lalist_mem_map.map[0].mem_start;
	mem_size = lalist_mem_map.map[0].mem_size;
	memblock_reserve(mem_start,mem_size);
}

void __init phy_pool_init(void)
{
	// reserve_phy_pool.paddr_start = memblock.reserved.regions[0].base;
	// reserve_phy_pool.btmp.btmp_bytes_len = (memblock.reserved.regions[0].size >> 12);
	// reserve_phy_pool.btmp.bits = (u8*)(RESERVED_PHY_POOL_BASE | DMW_MASK);
	reserve_phy_pool.paddr_start = (u64)(0x6000 | DMW_MASK);
	reserve_phy_pool.btmp.btmp_bytes_len = (u64)((0x8000000 - 0x6000) >> 12);
	reserve_phy_pool.btmp.bits = (u8*)(RESERVED_PHY_POOL_BASE | DMW_MASK);

	usr_phy_pool.paddr_start = memblock.memory.regions[0].base;
	usr_phy_pool.btmp.btmp_bytes_len = (memblock.memory.regions[0].size >> 12);
	usr_phy_pool.btmp.bits = (u8*)(USR_PHY_POOL_BASE | DMW_MASK);
}

void __init page_setting_init(void)
{
	unsigned long pwctl0, pwctl1;
	unsigned long pgd_i = 0, pgd_w = 0;
	unsigned long pud_i = 0, pud_w = 0;
	unsigned long pmd_i = 0, pmd_w = 0;
	unsigned long pte_i = 0, pte_w = 0;

	pgd_i = PGDIR_SHIFT;
	pgd_w = PAGE_SHIFT - 3;
	pmd_i = PMD_SHIFT;
	pmd_w = PAGE_SHIFT - 3;
	pte_i = PAGE_SHIFT;
	pte_w = PAGE_SHIFT - 3;

	pwctl0 = pte_i | pte_w << 5 | pmd_i << 10 | pmd_w << 15;
	pwctl1 = pgd_i | pgd_w << 6;

	csr_write64(pwctl0, LOONGARCH_CSR_PWCTL0);
	csr_write64(pwctl1, LOONGARCH_CSR_PWCTL1);

	// csr_write64(PGD_BASE_ADD,LOONGARCH_CSR_PGDL);
	// csr_write64(PGD_BASE_ADD,LOONGARCH_CSR_PGDH);
}