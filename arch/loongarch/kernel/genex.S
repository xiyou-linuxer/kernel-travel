#include <asm/stackframe.h>
#include <asm/linkage.h>

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

.section .data, "aw"
	.align	3
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
