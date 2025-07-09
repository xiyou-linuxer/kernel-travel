#ifndef _ASM_SBI_H
#define _ASM_SBI_H
/* */

struct sbiret {
	long error;
	long value;
};
// #ifdef CONFIG_RISCV_SBI
enum sbi_ext_id {
	SBI_EXT_0_1_SET_TIMER = 0x0,

};

// #endif/* CONFIG_RISCV_SBI */

struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
			unsigned long arg1, unsigned long arg2,
			unsigned long arg3, unsigned long arg4,
			unsigned long arg5);
#endif