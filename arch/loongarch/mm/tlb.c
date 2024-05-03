#include <linux/types.h>
#include <linux/stdio.h>
#include <linux/thread.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <asm/loongarch.h>
#include <asm/tlb.h>
#include <asm/page.h>
#include <asm/setup.h>


#define VADDR_INDEX_MASK _ULCAST_(0xffff)
char asid_bitmap[1 << (LOONGARCH64_ASIDBITS - 3)];

u64 tlb_search(u64 vaddr)
{
	struct task_struct *curr = running_thread();
	u64 asid = (read_csr_asid() & CSR_ASID_ASID);
	if(asid != curr->asid)
		goto error;
	
	csr_write64(vaddr,LOONGARCH_CSR_TLBEHI);
	__tlb_probe();
	u64 tlbidx = csr_read64(LOONGARCH_CSR_TLBIDX);
	if (tlbidx & CSR_TLBIDX_EHINV) {
		printk("Tlb miss");
		return 0;
	}
	return (tlbidx & CSR_TLBIDX_IDX);

error:
	printk("[Error]:curr->asid not equal to csr.asid !");
	return 0;
}

struct tlb_entry tlb_read(u64 index,u32 ps)
{
	struct tlb_entry entry;
	// tlbidx = (tlbidx & ~CSR_TLBIDX_IDX) | (index & CSR_TLBIDX_IDX);
	u64 tlbidx = (index << CSR_TLBIDX_IDX_SHIFT) 	|
		(0 << CSR_TLBIDX_EHINV_SHIFT)	|
		(ps << CSR_TLBIDX_PS_SHIFT);
	csr_write64(tlbidx,LOONGARCH_CSR_TLBIDX);
	__tlb_read();

	u64 ne = tlbidx & CSR_TLBIDX_EHINV;
	if(ne) {
		// 无效的 TLB
		printk("invalid TLB");
		// 后续进行错误处理
	} else {
		entry.index = index;
		entry.ehi = csr_read64(LOONGARCH_CSR_TLBEHI);
		entry.elo0 = csr_read64(LOONGARCH_CSR_TLBELO0);
		entry.elo1 = csr_read64(LOONGARCH_CSR_TLBELO1);
		entry.ps = (tlbidx & CSR_TLBIDX_PS) >> CSR_TLBIDX_PS_SHIFT;
	}
	return entry;
}

// is_vaild 是 CSR.TLBIDX.NE
s16 tlb_write(struct tlb_entry *entry, u8 is_valid)
{
	if(is_valid > 2 || entry == NULL)
		goto error;
	
	csr_write64(entry->ehi,LOONGARCH_CSR_TLBEHI);
	csr_write64(entry->elo0,LOONGARCH_CSR_TLBELO0);
	csr_write64(entry->elo1,LOONGARCH_CSR_TLBELO1);
	u64 tlbidx = (entry->index << CSR_TLBIDX_IDX_SHIFT) 	|
		(is_valid << CSR_TLBIDX_EHINV_SHIFT)		|
		(entry->ps << CSR_TLBIDX_PS_SHIFT);
	csr_write64(tlbidx,LOONGARCH_CSR_TLBIDX);
	__tlb_write_indexed();
	printk("index = 0x%x \nehi = 0x%x \nelo0 = 0x%x \nelo1 = 0x%x\n",entry->index,entry->ehi,entry->elo0,entry->elo1);
	
	return 0;
error:
	/* 打印错误信息 */
	printk("invalid arguments");
	return -1;
}

void __update_tlb(void)
{
	struct tlb_entry entry = tlb_read(0,12);

}

// 找到位图中值为0的位数
int find_free_asid(void) {
    int i, j;
    for (i = 0; i < (1 << (LOONGARCH64_ASIDBITS - 3)); i++) {
        if (asid_bitmap[i] != 0xFF) { // 如果字节不全为1
            for (j = 0; j < 8; j++) {
                if (!((asid_bitmap[i] >> j) & 1)) { // 找到第一个为0的位
                    return i * 8 + j;
                }
            }
        }
    }
    return -1; // 如果没有找到空闲的ASID位，则返回-1
}

// 设置指定的ASID位
void set_asid(uint32_t asid) {
    asid_bitmap[asid / 8] |= (1 << (asid % 8));
}

// 清除指定的ASID位
void clear_asid(uint32_t asid) {
    asid_bitmap[asid / 8] &= ~(1 << (asid % 8));
}

// 检查指定的ASID位是否被设置
int check_asid(uint32_t asid) { 
    return (asid_bitmap[asid / 8] & (1 << (asid % 8))) != 0;
}

// FOR TESTING
struct tlb_entry entity;

void test_tlb_func(void)
{
	entity.index = 0;
	entity.ehi = 0x1234 << 12;
	entity.elo0 = 0x1 | 0x3 << 2 | 0x1 <<6;
	entity.elo1 = 0x1 | 0x3 << 2 | 0x1 <<6;
	entity.ps = 12;
	s16 label = tlb_write(&entity,0);

	struct tlb_entry read_tlb = tlb_read(0,12);
	printk("index = 0x%x \nehi = 0x%x \nelo0 = 0x%x \nelo1 = 0x%x",read_tlb.index,read_tlb.ehi,read_tlb.elo0,read_tlb.elo1);

	u64 res = tlb_search(entity.ehi);
	printk("res = 0x%x \n",res);
}

static void page_setting_init(void)
{
	unsigned long pwctl0, pwctl1;
	unsigned long pgd_i = 0, pgd_w = 0;
	// unsigned long pud_i = 0, pud_w = 0;
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

	csr_write64(PGD_BASE_ADD,LOONGARCH_CSR_PGDL);
	csr_write64(PGD_BASE_ADD,LOONGARCH_CSR_PGDH);
}

static void setup_tlb_handler(int cpu)
{
	page_setting_init();
	// 刷新当前TLB
	invtlb(INVTLB_ALL,0,0);
	// if(cpu == 0) {
		// memcpy((void*)tlbrentry,handle_tlb_refill,0x80);
		// local_flush_icache_range(tlbrentry, tlbrentry + 0x80);
		// // TLB加载、TLB读取、TLB写入和TLB修改异常处理程序
		// set_handler(EXCCODE_TLBI * VECSIZE, handle_tlb_load, VECSIZE);
		// set_handler(EXCCODE_TLBL * VECSIZE, handle_tlb_load, VECSIZE);
		// set_handler(EXCCODE_TLBS * VECSIZE, handle_tlb_store, VECSIZE);
		// set_handler(EXCCODE_TLBM * VECSIZE, handle_tlb_modify, VECSIZE);
		// // 试图修改只读或受保护的TLB条目时触发的异常
		// set_handler(EXCCODE_TLBNR * VECSIZE, handle_tlb_protect, VECSIZE);
		// // 没有映射到物理地址的虚拟地址时触发的异常
		// set_handler(EXCCODE_TLBNX * VECSIZE, handle_tlb_protect, VECSIZE);
		// // TLB权限异常处理程序
		// set_handler(EXCCODE_TLBPE * VECSIZE, handle_tlb_protect, VECSIZE);

	// }
}

void tlb_init(int cpu)
{
	write_csr_stlbpgsize(PS_DEFAULT_SIZE);
	write_csr_stlbpgsize(PS_DEFAULT_SIZE);
	write_csr_tlbrefill_pagesize(PS_DEFAULT_SIZE);

	setup_tlb_handler(cpu);
}