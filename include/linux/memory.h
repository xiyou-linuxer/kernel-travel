#ifndef __MEMORY_H
#define __MEMORY_H
#include <stdint.h>
#include <asm/loongarch.h>

#define ENTRY_SIZE 8
#define PDE_IDX(addr) ((addr & 0x3fe00000)>>21)
#define PTE_IDX(addr) ((addr & 0x001ff000)>>12)

#define PTE_V (1UL << 0)
#define PTE_D (1UL << 1)
#define PTE_PLV (3UL << 2)

#define DMW_MASK CSR_DMW1_BASE

static inline void invalidate(void)
{
	asm volatile("invtlb 0x0,$r0,$r0");
}

void mm_init(void);
uint64_t get_page(void);
void page_table_add(uint64_t pd,uint64_t _vaddr,uint64_t _paddr,uint64_t attr);


#endif
