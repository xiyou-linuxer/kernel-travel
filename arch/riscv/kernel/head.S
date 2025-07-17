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
#include <asm/mm.h>

	__HEAD

SYM_CODE_START(kernel_entry)

	j _start_kernel

    .balign 8
    .dword 0x200000
    .ascii RISCV_IMAGE_MAGIC
    .balign 4

	/*等待中断*/
	.Lsecondary_park:
	/* We lack SMP support or have too many harts, so park this hart */
	wfi
	j .Lsecondary_park

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

 /*次核心睡觉*/
#ifdef CONFIG_SMP
        li t0, CONFIG_NR_CPUS
        blt a0, t0, .Lgood_cores
        tail .Lsecondary_park
.Lgood_cores:
#endif

/*清空bss段，写为0*/
#ifndef CONFIG_XIP_KERNEL
	/* Clear BSS for flat non-ELF images */
	la a3, __bss_start
	la a4, __bss_stop
	ble a4, a3, .Lclear_bss_done
.Lclear_bss:
	REG_S zero, (a3)
	add a3, a3, RISCV_SZPTR
	blt a3, a4, .Lclear_bss
.Lclear_bss_done:
#endif

# 把设备树地址__dtb_start放进setup_vm
la a0, __dtb_start
call setup_vm

# call setup_trap_vector
	tail start_kernel
	
SYM_CODE_END(_start_kernel)
	.section .bss
	.global stack0
stack0:
	.skip 4096 * 8

#setup_trap_vector:
	/* Set trap vector to exception handler */
	# la a0, handle_exception
	# csrw CSR_TVEC, a0

	/*
	 * Set sup0 scratch register to 0, indicating to exception vector that
	 * we are presently executing in kernel.
	 */
	# csrw CSR_SCRATCH, zero
	# ret
