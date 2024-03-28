#ifndef _ASM_PTRACE_H
#define _ASM_PTRACE_H

#include <linux/compiler_attributes.h>

/*
 * This struct defines the way the registers are stored on the stack during
 * a system call/exception. If you add a register here, please also add it to
 * regoffset_table[] in arch/loongarch/kernel/ptrace.c.
 */
struct pt_regs {
	/* Main processor registers. */
	unsigned long regs[32];

	/* Original syscall arg0. */
	unsigned long orig_a0;

	/* Special CSR registers. */
	unsigned long csr_era;
	unsigned long csr_badvaddr;
	unsigned long csr_crmd;
	unsigned long csr_prmd;
	unsigned long csr_euen;
	unsigned long csr_ecfg;
	unsigned long csr_estat;
	unsigned long __last[];
} __aligned(8);

#endif /* _ASM_PTRACE_H */
