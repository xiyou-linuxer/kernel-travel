#ifndef _ASM_PROCESS_H
#define _ASM_PROCESS_H

struct thread_stack {
	/* Main processor registers. */
	unsigned long reg01, reg03, reg22; /* ra sp fp */
	unsigned long reg23, reg24, reg25, reg26; /* s0-s3 */
	unsigned long reg27, reg28, reg29, reg30, reg31; /* s4-s8 */

	/* __schedule() return address / call frame address */
	unsigned long sched_ra;
	unsigned long sched_cfa;

	/* CSR registers */
	unsigned long csr_prmd;
	unsigned long csr_crmd;
	unsigned long csr_euen;
	unsigned long csr_ecfg;
	unsigned long csr_badvaddr; /* Last user fault */

	/* Scratch registers */
	unsigned long scr0;
	unsigned long scr1;
	unsigned long scr2;
	unsigned long scr3;

	/* Eflags register */
	unsigned long eflags;
};

#endif /* _ASM_PROCESS_H */
