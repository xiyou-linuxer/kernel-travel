#include <linux/types.h>
#include <linux/stdio.h>
#include <linux/printk.h>
#include <asm/loongarch.h>
#include <asm/tlb.h>

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
		entry.ps = tlbidx & CSR_TLBIDX_PS;
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
int find_free_asid() {
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
	entity.index = 1;
	entity.ehi = 0x1234 << 12;
	entity.elo0 = 0x1 | 0x3 << 2 | 0x1 <<6;
	entity.elo1 = 0x1 | 0x3 << 2 | 0x1 <<6;
	entity.ps = 12;
	s16 label = tlb_write(&entity,0);

	struct tlb_entry read_tlb = tlb_read(1,12);
	printk("index = 0x%x \nehi = 0x%x \nelo0 = 0x%x \nelo1 = 0x%x",read_tlb.index,read_tlb.ehi,read_tlb.elo0,read_tlb.elo1);
}