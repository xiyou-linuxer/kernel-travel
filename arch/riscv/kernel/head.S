/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <xkernel/init.h>
#include <xkernel/linkage.h>

	__HEAD

SYM_CODE_START(kernel_entry)
lable_1:
	j lable_1
	/* reserved */
SYM_CODE_END(kernel_entry)
