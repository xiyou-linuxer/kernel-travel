#ifndef _ASM_STACKFRAME_H
#define _ASM_STACKFRAME_H

	.macro	set_saved_sp stackp temp temp2
	la.abs	  \temp, kernelsp
#ifdef CONFIG_SMP
	LONG_ADD  \temp, \temp, u0
#endif
	LONG_S	  \stackp, \temp, 0
	.endm

#endif /* _ASM_STACKFRAME_H */
