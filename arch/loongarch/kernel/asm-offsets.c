/*
 * Copyright (C) 1995-2003 Russell King
 *               2001-2002 Keith Owens
 *     
 * Generate definitions needed by assembly language modules.
 * This code generates raw asm output which is post-processed to extract
 * and format the required data.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/compiler.h>
#include <linux/kbuild.h>

#include <asm/page.h>
int main(void)
{
	BLANK();
	DEFINE(PAGE_SZ, PAGE_SIZE);
	BLANK();

// 	DEFINE (__TIF_SINGLESTEP, PROCFLAG_SINGLESTEP);
// 	DEFINE(S_FRAME_SIZE, sizeof(struct exception_spot));
// 	DEFINE(MM_CONTEXT_ID,		offsetof(struct memory_map_desc, map_context.asid));
// 	DEFINE(S_X0,			offsetof(struct exception_spot, regs[0]));
// 	DEFINE(S_X1,			offsetof(struct exception_spot, regs[1]));
// 	DEFINE(S_X2,			offsetof(struct exception_spot, regs[2]));
// 	DEFINE(S_X3,			offsetof(struct exception_spot, regs[3]));
// 	DEFINE(S_X4,			offsetof(struct exception_spot, regs[4]));
// 	DEFINE(S_X5,			offsetof(struct exception_spot, regs[5]));
// 	DEFINE(S_X6,			offsetof(struct exception_spot, regs[6]));
// 	DEFINE(S_X7,			offsetof(struct exception_spot, regs[7]));
// 	DEFINE(S_LR,			offsetof(struct exception_spot, regs[30]));
// 	DEFINE(S_SP,			offsetof(struct exception_spot, sp));
// #ifdef CONFIG_COMPAT
// 	DEFINE(S_COMPAT_SP,		offsetof(struct exception_spot, compat_sp));
// #endif
// 	DEFINE(S_PSTATE,		offsetof(struct exception_spot, pstate));
// 	DEFINE(S_PC,			offsetof(struct exception_spot, pc));
// 	DEFINE(S_ORIG_X0,		offsetof(struct exception_spot, orig_x0));
// 	DEFINE(S_SYSCALLNO,		offsetof(struct exception_spot, syscallno));

// 	DEFINE(S_FRAME_SIZE,		sizeof(struct exception_spot));
// 	DEFINE(TI_PREEMPT,		offsetof(struct process_desc, preempt_count));
// 	DEFINE(TI_FLAGS,		offsetof(struct process_desc, flags));

// 	DEFINE(THREAD_CPU_CONTEXT,	offsetof(struct task_desc, task_spot.cpu_context));

	return 0; 
}
