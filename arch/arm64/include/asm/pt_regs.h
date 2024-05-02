#ifndef __ASM_PT_REGS_H
#define __ASM_PT_REGS_H

#include <uapi/asm/ptrace.h>

#ifndef __ASSEMBLY__

/*
 * This struct defines the way the registers are stored on the stack during an
 * exception. Note that sizeof(struct pt_regs) has to be a multiple of 16 (for
 * stack alignment). struct user_pt_regs must form a prefix of struct pt_regs.
 */
struct pt_regs {
	union {
		struct user_pt_regs user_regs;
		struct {
			u64 regs[31];
			u64 sp;
			u64 pc;
			u64 pstate;
		};
	};
	u64 orig_x0;
	u64 syscallno;
};
#endif /* __ASSEMBLY__ */

#endif /* __ASM_PT_REGS_H */
