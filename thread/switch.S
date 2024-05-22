#include <asm/regdef.h>
#include <asm/stackframe.h>
#include <asm/asm-offsets.h>
#include <asm/asmmacro.h>
#include <asm/bootinfo.h>

.section ".text","ax"

	.align 12

.globl switch_to
.globl prepare_switch
.globl irq_count
switch_to:
	csrrd	t1, LOONGARCH_CSR_PRMD
	stptr.d	t1, a0, THREAD_CSRPRMD

	cpu_save_nonscratch a0
	stptr.d	ra, a0, THREAD_REG01

	cpu_restore_nonscratch a1
	li.w   t0, THREAD_SIZE
	add.d  t0, t0, a1
	set_saved_sp t0,t1

	ldptr.d	t1, a1, THREAD_CSRPRMD
	csrwr	t1, LOONGARCH_CSR_PRMD

	jr	ra

prepare_switch:
	csrrd	t1, LOONGARCH_CSR_PRMD
	stptr.d	t1, a0, THREAD_CSRPRMD

	cpu_save_nonscratch a0
	stptr.d	a1, a0, THREAD_REG01
	stptr.d a2, a0, THREAD_REG03

	jr ra
