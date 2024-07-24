#include <asm/asm.h>
#include <asm/loongarch.h>
#include <asm/stackframe.h>
#include <xkernel/linkage.h>

.macro la_abs reg, sym
	la.abs	\reg, \sym
.endm

.macro tlb_do_page_fault, write
SYM_CODE_START(tlb_do_page_fault_\write)
	SAVE_ALL
	csrrd	a2, LOONGARCH_CSR_BADV
	move	a0, sp
	REG_S	a2, sp, PT_BVADDR
	li.w	a1, \write
	bl	do_page_fault
	RESTORE_ALL_AND_RET
SYM_CODE_END(tlb_do_page_fault_\write)
.endm

tlb_do_page_fault 0
tlb_do_page_fault 1

SYM_CODE_START(handle_tlb_refill)
	csrwr	t0, LOONGARCH_CSR_TLBRSAVE
	csrrd	t0, LOONGARCH_CSR_PGD
	lddir	t0, t0, 4
	lddir	t0, t0, 2
	lddir	t0, t0, 1
	ldpte	t0, 0
	ldpte	t0, 1
	tlbfill
	csrrd	t0, LOONGARCH_CSR_TLBRSAVE
	ertn
SYM_CODE_END(handle_tlb_refill)

SYM_CODE_START(handle_tlb_load_ptw)
	csrwr	t0, LOONGARCH_CSR_KS0
	csrwr	t1, LOONGARCH_CSR_KS1
	la_abs	t0, tlb_do_page_fault_0
	jr	t0
SYM_CODE_END(handle_tlb_load_ptw)

SYM_CODE_START(handle_tlb_store_ptw)
	csrwr	t0, LOONGARCH_CSR_KS0
	csrwr	t1, LOONGARCH_CSR_KS1
	la_abs	t0, tlb_do_page_fault_1
	jr	t0
SYM_CODE_END(handle_tlb_store_ptw)

SYM_CODE_START(handle_tlb_modify_ptw)
	csrwr	t0, LOONGARCH_CSR_KS0
	csrwr	t1, LOONGARCH_CSR_KS1
	la_abs	t0, tlb_do_page_fault_1
	jr	t0
SYM_CODE_END(handle_tlb_modify_ptw)