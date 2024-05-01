#include <asm/stackframe.h>
#include <asm/linkage.h>
#include <asm/asm.h>
#include <asm/regdef.h>

	.extern do_syscall

	.macro	BUILD_VI_HANDLER num
	.align	5
SYM_FUNC_START(handle_vint_\num)
	BACKUP_T0T1
	SAVE_ALL
	addi.d	t0, zero, \num
	cfi_st	t0, PT_HWIRQ, 0
	move	a0, sp
	move	a1, sp
	la.abs	t0, do_vint
	jirl	ra, t0, 0
	RESTORE_ALL_AND_RET
SYM_FUNC_END(handle_vint_\num)
	.endm


	BUILD_VI_HANDLER 0
	BUILD_VI_HANDLER 1
	BUILD_VI_HANDLER 2
	BUILD_VI_HANDLER 3
	BUILD_VI_HANDLER 4
	BUILD_VI_HANDLER 5
	BUILD_VI_HANDLER 6
	BUILD_VI_HANDLER 7
	BUILD_VI_HANDLER 8
	BUILD_VI_HANDLER 9
	BUILD_VI_HANDLER 10
	BUILD_VI_HANDLER 11
	BUILD_VI_HANDLER 12
	BUILD_VI_HANDLER 13

user_ret:
    RESTORE_ALL_AND_RET


SYM_FUNC_START(syscall_handler)
	la.abs	sp, kernelsp

	addi.d	sp, sp, -PT_SIZE
	cfi_st	t2, PT_R3
	st.d	zero, sp, PT_R0
	csrrd	t2, LOONGARCH_CSR_PRMD
	st.d	t2, sp, PT_PRMD
	csrrd	t2, LOONGARCH_CSR_CRMD
	st.d	t2, sp, PT_CRMD
	csrrd	t2, LOONGARCH_CSR_EUEN
	st.d	t2, sp, PT_EUEN
	csrrd	t2, LOONGARCH_CSR_ECFG
	st.d	t2, sp, PT_ECFG
	csrrd	t2, LOONGARCH_CSR_ESTAT
	st.d	t2, sp, PT_ESTAT
	cfi_st	ra, PT_R1
	cfi_st	a0, PT_R4
	cfi_st	a1, PT_R5
	cfi_st	a2, PT_R6
	cfi_st	a3, PT_R7
	cfi_st	a4, PT_R8
	cfi_st	a5, PT_R9
	cfi_st	a6, PT_R10
	cfi_st	a7, PT_R11

	csrrd	ra, LOONGARCH_CSR_ERA
	st.d	ra, sp, PT_ERA

	cfi_st	tp, PT_R2
	cfi_st	u0, PT_R21
	cfi_st	fp, PT_R22
	SAVE_STATIC

	move	a0,sp
	bl	do_syscall
	RESTORE_ALL_AND_RET
SYM_FUNC_END(syscall_handler)


SYM_FUNC_START(tlb_handler)
    csrwr t0,LOONGARCH_CSR_TLBRSAVE
    csrrd t0,LOONGARCH_CSR_PGD
    lddir t0,t0,3
    lddir t0,t0,1
    ldpte t0,0
    ldpte t0,1
    tlbfill
    csrrd t0,LOONGARCH_CSR_TLBRSAVE
    ertn
SYM_FUNC_END(tlb_handler)

.section .data, "aw"
	.align	3
		.globl  user_ret
		.globl  syscall_handler
		.globl  tlb_handler
		.globl  vector_table
vector_table:
	PTR	handle_vint_0
	PTR	handle_vint_1
	PTR	handle_vint_2
	PTR	handle_vint_3
	PTR	handle_vint_4
	PTR	handle_vint_5
	PTR	handle_vint_6
	PTR	handle_vint_7
	PTR	handle_vint_8
	PTR	handle_vint_9
	PTR	handle_vint_10
	PTR	handle_vint_11
	PTR	handle_vint_12
	PTR	handle_vint_13
