#ifndef __MEMORY_H
#define __MEMORY_H
#include <linux/types.h>
#include <bitmap.h>

#define ENTRY_SIZE 8
#define PGD_IDX(addr) ((addr & 0x6fc0000000)>>30)
#define PMD_IDX(addr) ((addr & 0x003fe00000)>>21)
#define PTE_IDX(addr) ((addr & 0x00001ff000)>>12)

#define PTE_V (1UL << 0)
#define PTE_D (1UL << 1)
#define PTE_PLV (3UL << 2)

#define DMW_MASK  (0x9000000000000000)
struct pool {
	u64 paddr_start;
	struct bitmap btmp;
};

struct mm_struct {
	u64 pgd;
};

extern struct pool reserve_phy_pool;
extern struct pool phy_pool;

static inline void invalidate(void)
{
	asm volatile("invtlb 0x0,$r0,$r0");
}

uint64_t get_page(void);
unsigned long get_kernel_pge(void);
void page_table_add(uint64_t pd,uint64_t _vaddr,uint64_t _paddr,uint64_t attr);

void * kmalloc(u64 size);

#endif
