#ifndef _ASM_TLB_H
#define _ASM_TLB_H

#include <linux/types.h>

#define LOONGARCH64_ASIDBITS 10

enum invtlb_ops {
	/* Invalid all tlb */
	INVTLB_ALL = 0x0,
	/* Invalid current tlb */
	INVTLB_CURRENT_ALL = 0x1,
	/* Invalid all global=1 lines in current tlb */
	INVTLB_CURRENT_GTRUE = 0x2,
	/* Invalid all global=0 lines in current tlb */
	INVTLB_CURRENT_GFALSE = 0x3,
	/* Invalid global=0 and matched asid lines in current tlb */
	INVTLB_GFALSE_AND_ASID = 0x4,
	/* Invalid addr with global=0 and matched asid in current tlb */
	INVTLB_ADDR_GFALSE_AND_ASID = 0x5,
	/* Invalid addr with global=1 or matched asid in current tlb */
	INVTLB_ADDR_GTRUE_OR_ASID = 0x6,
	/* Invalid matched gid in guest tlb */
	INVGTLB_GID = 0x9,
	/* Invalid global=1, matched gid in guest tlb */
	INVGTLB_GID_GTRUE = 0xa,
	/* Invalid global=0, matched gid in guest tlb */
	INVGTLB_GID_GFALSE = 0xb,
	/* Invalid global=0, matched gid and asid in guest tlb */
	INVGTLB_GID_GFALSE_ASID = 0xc,
	/* Invalid global=0 , matched gid, asid and addr in guest tlb */
	INVGTLB_GID_GFALSE_ASID_ADDR = 0xd,
	/* Invalid global=1 , matched gid, asid and addr in guest tlb */
	INVGTLB_GID_GTRUE_ASID_ADDR = 0xe,
	/* Invalid all gid gva-->gpa guest tlb */
	INVGTLB_ALLGID_GVA_TO_GPA = 0x10,
	/* Invalid all gid gpa-->hpa tlb */
	INVTLB_ALLGID_GPA_TO_HPA = 0x11,
	/* Invalid all gid tlb, including  gva-->gpa and gpa-->hpa */
	INVTLB_ALLGID = 0x12,
	/* Invalid matched gid gva-->gpa guest tlb */
	INVGTLB_GID_GVA_TO_GPA = 0x13,
	/* Invalid matched gid gpa-->hpa tlb */
	INVTLB_GID_GPA_TO_HPA = 0x14,
	/* Invalid matched gid tlb,including gva-->gpa and gpa-->hpa */
	INVTLB_GID_ALL = 0x15,
	/* Invalid matched gid and addr gpa-->hpa tlb */
	INVTLB_GID_ADDR = 0x16,
};

struct tlb_entry {
	u64 index;
	u64 ehi;	// 表项高位
	u64 elo0;	// 表项低位
	u64 elo1;	
	u64 ps;		// 存放的页表项的页大小
};

/*
 * TLB Invalidate Flush
 */
static inline void tlbclr(void)
{
	__asm__ __volatile__("tlbclr");
}

static inline void tlbflush(void)
{
	__asm__ __volatile__("tlbflush");
}

/*
 * TLB R/W operations.
 */
static inline void tlb_probe(void)
{
	__asm__ __volatile__("tlbsrch");
}

static inline void __tlb_read(void)
{
	__asm__ __volatile__("tlbrd");
}

static inline void __tlb_write_indexed(void)
{
	__asm__ __volatile__("tlbwr");
}

static inline void tlb_write_random(void)
{
	__asm__ __volatile__("tlbfill");
}

/*
op表示操作类型，下面是loongarch手册中列出的op类型：
op=0：清除所有表项。
op=1：清除所有表项。效果和op=0完全一致。
op=2：清除所有G=1的表项。
op=3：清除所有G=0的表项。
op=4：清除所有G=0，且ASID等于寄存器指定ASID的表项。
op=5：清除所有G=0，ASID等于寄存器指定ASID，且VA等于寄存器指定VA的表项。
op=6：清除所有G=1或ASID等于寄存器指定ASID，且VA等于寄存器指定VA的表项。
通用寄存器info中存放ASID信息。当op对应的操作不需要ASID时，info应设置为r0。
通用寄存器addr中存放VA虚拟地址信息。当op对应的操作不需要VA时，rk应设置为r0。 */

static inline void invtlb(u32 op, u32 info, u64 addr)
{
	__asm__ __volatile__(
		"invtlb %0, %1, %2\n\t"
		:
		: "i"(op), "r"(info), "r"(addr)
		: "memory"
		);
}


void __update_tlb(void);
struct tlb_entry tlb_read(u64 index,u32 ps);
s16 tlb_write(struct tlb_entry *entry, u8 is_valid);

void test_tlb_func(void);

// 定义一个char类型的位图数组，数组大小为 (1024 / 8) = 128
char asid_bitmap[1 << (LOONGARCH64_ASIDBITS - 3)];

int find_free_asid();
void set_asid(uint32_t asid);
void clear_asid(uint32_t asid);
int check_asid(uint32_t asid);

#endif