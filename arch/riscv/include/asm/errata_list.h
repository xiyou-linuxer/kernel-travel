#ifndef ASM_ERRATA_LIST_H
#define ASM_ERRATA_LIST_H

#ifdef CONFIG_ERRATA_THEAD_MAE
/*
 * IO/NOCACHE memory types are handled together with svpbmt,
 * so on T-Head chips, check if no other memory type is set,
 * and set the non-0 PMA type if applicable.
 * IO/NOCACHE 内存类型与 svpbmt 一起处理，
 * 因此在 T-Head 芯片上，请检查是否未设置其他内存类型，
 * 并设置非 0 PMA 类型（如果适用）。
 */
#define ALT_THEAD_PMA(_val)						\
asm volatile(ALTERNATIVE(						\
	__nops(7),							\
	"li      t3, %1\n\t"						\
	"slli    t3, t3, %3\n\t"					\
	"and     t3, %0, t3\n\t"					\
	"bne     t3, zero, 2f\n\t"					\
	"li      t3, %2\n\t"						\
	"slli    t3, t3, %3\n\t"					\
	"or      %0, %0, t3\n\t"					\
	"2:",  THEAD_VENDOR_ID,						\
		ERRATA_THEAD_MAE, CONFIG_ERRATA_THEAD_MAE)		\
	: "+r"(_val)							\
	: "I"(_PAGE_MTMASK_THEAD >> ALT_THEAD_MAE_SHIFT),		\
	  "I"(_PAGE_PMA_THEAD >> ALT_THEAD_MAE_SHIFT),			\
	  "I"(ALT_THEAD_MAE_SHIFT)					\
	: "t3")
#else
#define ALT_THEAD_PMA(_val)
#endif

#endif

