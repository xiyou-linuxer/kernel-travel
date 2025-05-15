#include <xkernel/init.h>
#include <xkernel/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/thread_info.h>
#include <asm/image.h>
#include <asm/asm.h>
#include <asm/elf.h>
#include <asm/csr.h>
#include <asm/page.h>
#include <asm/setup.h>

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
	load_global_pointer
	
	/*禁用浮点单元*/
	li t0, SR_FS_VS
	csrc CSR_STATUS, t0

	
	sb a1, 0(a0)
	la a4, __bss_stop
	ble a4, a3, clear_bss_done
clear_bss:
	REG_S zero, (a3)
	add a3, a3, RISCV_SZPTR
	blt a3, a4, clear_bss
clear_bss_done:

	/* 保存硬件线程（Hart）ID 和设备树（DTB）物理地址 */
	mv s0, a0
	mv s1, a1
	la a2, boot_cpu_hartid
	REG_S a0, (a2)

	/*
	设置页表并跳转到虚拟地址
	la sp, init_thread_union + THREAD_SIZE
	*/
	la a0, init_thread_union  # 加载 init_thread_union 的地址到寄存器 a0
	li t0, 16384              # 将偏移量 16384 加载到寄存器 t0
	add a0, a0, t0            # 将偏移量加到 a0
	mv sp, a0                 # 将最终地址加载到 sp
	mv a0, s1
	call setup_vm

	la a0, early_pg_dir
	# call relocate

	tail start_kernel
	
SYM_CODE_END(_start_kernel)
