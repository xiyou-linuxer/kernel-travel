#include <asm/regdef.h>
#include <asm/stackframe.h>
#include <asm/asm-offsets.h>
#include <asmmacro.h>

.section ".text","ax"

	.align 12

.global switch_to
switch_to:
	csrrd	t1, LOONGARCH_CSR_PRMD
	stptr.d	t1, a0, THREAD_CSRPRMD

	cpu_save_nonscratch a0
	stptr.d	ra, a0, THREAD_REG01

	cpu_restore_nonscratch a1

    ldptr.d ra, a1, THREAD_REG01
	ldptr.d	t1, a1, THREAD_CSRPRMD
	csrwr	t1, LOONGARCH_CSR_PRMD

    ;xor ra,ra,ra
    ;addi.d ra,ra,186
	jr	ra
