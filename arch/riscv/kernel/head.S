/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <xkernel/linkage.h>

SYM_CODE_START(_start)
	/* jump to start kernel */
	j _start_kernel
	/* reserved */
	.word 0
SYM_CODE_END(_start)

SYM_CODE_START(_start_kernel)
	/* Mask all interrupts */
	csrw 0x304, zero
SYM_CODE_END(_start_kernel)
