#include <linux/memory.h>
#include <bitmap.h>
#include <asm/loongarch.h>
#include <stdint.h>
#include <debug.h>
#include <linux/stdio.h>
#include <linux/string.h>

#define PAGESIZE 4096

char memmap[1024];
struct pool {
    struct bitmap btmp;
    uint64_t paddr_start;
};

struct pool phy_pool;

void mm_init(void)
{
    phy_pool.paddr_start = 0;
    phy_pool.btmp.bits   = (uint8_t*)memmap;
    phy_pool.btmp.btmp_bytes_len = 1024;
    
    unsigned long pd_i,pd_w;
    unsigned long pt_i,pt_w;
    pd_i = 21;
    pd_w = 9;
    pt_i = 12;
    pt_w = 9;
    write_csr_pwcl(0 | (pd_w << 15) | (pd_i << 10) | (pt_w << 5) | (pt_i << 0));
    write_csr_pwch(0);
}

uint64_t get_page(void)
{
    unsigned long page;
    uint64_t bit_off = bit_scan(&phy_pool.btmp,1);
    if (bit_off == -1){
        return 0;
    }
    bitmap_set(&phy_pool.btmp,bit_off,1);
    printk("page %d allocated\n",bit_off);
    page = (bit_off << 12) | CSR_DMW1_BASE;
    memset((void*)page,0,PAGESIZE);

    return page;
}

uint64_t *pde_ptr(uint64_t pd,uint64_t vaddr)
{
    uint64_t v = pd + PDE_IDX(vaddr)*ENTRY_SIZE;
    printk("base:%#llx\n",CSR_DMW1_BASE);
    printk("v:%#llx\n",v | CSR_DMW1_BASE);
    return (uint64_t*)(v | CSR_DMW1_BASE);
}

uint64_t *pte_ptr(uint64_t pd,uint64_t vaddr)
{
    uint64_t pt;
    uint64_t *pde,*pte;
    pde = pde_ptr(pd,vaddr);
    printk("pde=%#llx\n",(uint64_t)pde);
    if (*pde)
    {
        pt = *pde | CSR_DMW1_BASE;
    }
    else
    {
        pt = get_page();
        *pde = pt;
        printk("*pde=%#llx\n",*pde);
    }
    pte = (uint64_t*)(pt + PTE_IDX(vaddr)*ENTRY_SIZE);

    return pte;
}


void page_table_add(uint64_t pd,uint64_t _vaddr,uint64_t _paddr,uint64_t attr)
{
    uint64_t *pte = pte_ptr(pd,_vaddr);
    if (*pte) {
        printk("page_table_add: try to remap!!!\n");
        BUG();
    }
    *pte = _paddr | attr;
    invalidate();
}

