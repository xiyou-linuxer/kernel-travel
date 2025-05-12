/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */
#include <xkernel/init.h>
#include <xkernel/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/thread_info.h>
#include <asm/asm.h>
#include <asm/csr.h>

	__HEAD

SYM_CODE_START(kernel_entry)

	li a0, 0x10000000
	li a1 ,'H'
	sb a1, 0(a0)

	li a1 ,'i'
	sb a1, 0(a0)

	li a1 ,'\n'
	sb a1, 0(a0)

	c.li s4,-13
	j _start_kernel

SYM_CODE_END(kernel_entry)

SYM_CODE_START(_start_kernel)
	
	/*系统启动初期或异常返回前，确保中断状态清空*/
	csrw CSR_IE, zero  /*Supervisor 中断使能寄存器*/
	csrw CSR_IP, zero  /*Supervisor 中断挂起寄存器*/

	/*用于初始化全局指针寄存器 gp（x3）*/
	load_global_pointer
	
	/*禁用浮点单元*/
	li t0, SR_FS_VS
	csrc CSR_STATUS, t0

	la sp, stack0
    li t0, 1024*4

    mv t1, a0
    addi t1, t1, 1
    mul t0, t0, t1
    add sp, sp, t0

	li a0, 0x10000000
	li a1 ,'X'
	sb a1, 0(a0)
	j .
	tail start_kernel
SYM_CODE_END(_start_kernel)

	.section .bss
	.global stack0
stack0:
	.skip 4096 * 8