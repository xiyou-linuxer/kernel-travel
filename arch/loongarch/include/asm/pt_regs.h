#ifndef _PT_REGS_H
#define _PT_REGS_H

/*
 * This struct defines the way the registers are stored on the stack during
 * a system call/exception. If you add a register here, please also add it to
 * regoffset_table[] in arch/loongarch/kernel/ptrace.c.
 */
struct pt_regs {
	/* hardware irq number */
	unsigned long hwirq;
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
} __attribute__((aligned (8)));

/*
 * Does the process account for user or for system time?
 */
#define user_mode(regs) (((regs)->csr_prmd & PLV_MASK) == PLV_USER)

#endif /* _PT_REGS_H */
