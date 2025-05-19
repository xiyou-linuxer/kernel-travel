/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <xkernel/init.h>
#include <xkernel/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/thread_info.h>
#include <asm/image.h>
#include <asm/asm.h>
#include <asm/elf.h>
#include <asm/csr.h>
#include <asm/page.h>
/*
#include <asm/thread_info.h>
#include <asm/asm-offsets.h>
#include <asm/asm.h>
#include <linux/init.h>
#include <linux/linkage.h>
#include <asm/page.h>
#include <asm/csr.h>
#include <asm/hwcap.h>
#include <asm/image.h>
*/
	__HEAD

SYM_CODE_START(kernel_entry)

	j _start_kernel

    .balign 8
    .dword 0x200000
    .ascii RISCV_IMAGE_MAGIC
    .balign 4

SYM_CODE_END(kernel_entry)

	__INIT
SYM_CODE_START(_start_kernel)
	
	/*系统启动初期或异常返回前，确保中断状态清空*/
	csrw CSR_IE, zero  /*Supervisor 中断使能寄存器*/
	csrw CSR_IP, zero  /*Supervisor 中断挂起寄存器*/

	/*用于初始化全局指针寄存器 gp（x3）*/
.option push
.option norelax
	la gp, __global_pointer$
.option pop

	la sp, stack0
    li t0, 1024*4
	
    addi t1, t1, 1
    mul t0, t0, t1
    add sp, sp, t0


	tail start_kernel
	
SYM_CODE_END(_start_kernel)
	.section .bss
	.global stack0
stack0:
	.skip 4096 * 8