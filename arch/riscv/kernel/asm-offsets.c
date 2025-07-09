#include <asm/processor.h>
#include <xkernel/kbuild.h>
#include <xkernel/thread.h>
#include <xkernel/sched.h>
#define ALIGN(x, a)		__ALIGN_KERNEL((x), (a))
void asm_offsets(void);

void asm_offsets(void)
{
    DEFINE(PT_SIZE_ON_STACK, ALIGN(sizeof(struct pt_regs), STACK_ALIGN));	
    OFFSET(TASK_THREAD_RA, task_struct, thread.ra);
	OFFSET(TASK_THREAD_SP, task_struct, thread.sp);
	OFFSET(TASK_THREAD_S0, task_struct, thread.s[0]);
	OFFSET(TASK_THREAD_S1, task_struct, thread.s[1]);
	OFFSET(TASK_THREAD_S2, task_struct, thread.s[2]);
	OFFSET(TASK_THREAD_S3, task_struct, thread.s[3]);
	OFFSET(TASK_THREAD_S4, task_struct, thread.s[4]);
	OFFSET(TASK_THREAD_S5, task_struct, thread.s[5]);
	OFFSET(TASK_THREAD_S6, task_struct, thread.s[6]);
	OFFSET(TASK_THREAD_S7, task_struct, thread.s[7]);
	OFFSET(TASK_THREAD_S8, task_struct, thread.s[8]);
	OFFSET(TASK_THREAD_S9, task_struct, thread.s[9]);
	OFFSET(TASK_THREAD_S10, task_struct, thread.s[10]);
	OFFSET(TASK_THREAD_S11, task_struct, thread.s[11]);
	OFFSET(TASK_TI_FLAGS, task_struct, thread_info.flags);
	OFFSET(TASK_TI_PREEMPT_COUNT, task_struct, thread_info.preempt_count);
    OFFSET(TASK_TI_KERNEL_SP, task_struct, thread_info.kernel_sp);
    OFFSET(TASK_TI_USER_SP, task_struct, thread_info.user_sp);


    OFFSET(TASK_THREAD_F0,  task_struct, thread.fstate.f[0]);
	OFFSET(TASK_THREAD_F1,  task_struct, thread.fstate.f[1]);
	OFFSET(TASK_THREAD_F2,  task_struct, thread.fstate.f[2]);
	OFFSET(TASK_THREAD_F3,  task_struct, thread.fstate.f[3]);
	OFFSET(TASK_THREAD_F4,  task_struct, thread.fstate.f[4]);
	OFFSET(TASK_THREAD_F5,  task_struct, thread.fstate.f[5]);
	OFFSET(TASK_THREAD_F6,  task_struct, thread.fstate.f[6]);
	OFFSET(TASK_THREAD_F7,  task_struct, thread.fstate.f[7]);
	OFFSET(TASK_THREAD_F8,  task_struct, thread.fstate.f[8]);
	OFFSET(TASK_THREAD_F9,  task_struct, thread.fstate.f[9]);
	OFFSET(TASK_THREAD_F10, task_struct, thread.fstate.f[10]);
	OFFSET(TASK_THREAD_F11, task_struct, thread.fstate.f[11]);
	OFFSET(TASK_THREAD_F12, task_struct, thread.fstate.f[12]);
	OFFSET(TASK_THREAD_F13, task_struct, thread.fstate.f[13]);
	OFFSET(TASK_THREAD_F14, task_struct, thread.fstate.f[14]);
	OFFSET(TASK_THREAD_F15, task_struct, thread.fstate.f[15]);
	OFFSET(TASK_THREAD_F16, task_struct, thread.fstate.f[16]);
	OFFSET(TASK_THREAD_F17, task_struct, thread.fstate.f[17]);
	OFFSET(TASK_THREAD_F18, task_struct, thread.fstate.f[18]);
	OFFSET(TASK_THREAD_F19, task_struct, thread.fstate.f[19]);
	OFFSET(TASK_THREAD_F20, task_struct, thread.fstate.f[20]);
	OFFSET(TASK_THREAD_F21, task_struct, thread.fstate.f[21]);
	OFFSET(TASK_THREAD_F22, task_struct, thread.fstate.f[22]);
	OFFSET(TASK_THREAD_F23, task_struct, thread.fstate.f[23]);
	OFFSET(TASK_THREAD_F24, task_struct, thread.fstate.f[24]);
	OFFSET(TASK_THREAD_F25, task_struct, thread.fstate.f[25]);
	OFFSET(TASK_THREAD_F26, task_struct, thread.fstate.f[26]);
	OFFSET(TASK_THREAD_F27, task_struct, thread.fstate.f[27]);
	OFFSET(TASK_THREAD_F28, task_struct, thread.fstate.f[28]);
	OFFSET(TASK_THREAD_F29, task_struct, thread.fstate.f[29]);
	OFFSET(TASK_THREAD_F30, task_struct, thread.fstate.f[30]);
	OFFSET(TASK_THREAD_F31, task_struct, thread.fstate.f[31]);
	OFFSET(TASK_THREAD_FCSR, task_struct, thread.fstate.fcsr);

    DEFINE(PT_SIZE, sizeof(struct pt_regs));
	OFFSET(PT_EPC, pt_regs, epc);
	OFFSET(PT_RA, pt_regs, ra);
	OFFSET(PT_FP, pt_regs, s0);
	OFFSET(PT_S0, pt_regs, s0);
	OFFSET(PT_S1, pt_regs, s1);
	OFFSET(PT_S2, pt_regs, s2);
	OFFSET(PT_S3, pt_regs, s3);
	OFFSET(PT_S4, pt_regs, s4);
	OFFSET(PT_S5, pt_regs, s5);
	OFFSET(PT_S6, pt_regs, s6);
	OFFSET(PT_S7, pt_regs, s7);
	OFFSET(PT_S8, pt_regs, s8);
	OFFSET(PT_S9, pt_regs, s9);
	OFFSET(PT_S10, pt_regs, s10);
	OFFSET(PT_S11, pt_regs, s11);
	OFFSET(PT_SP, pt_regs, sp);
	OFFSET(PT_TP, pt_regs, tp);
	OFFSET(PT_A0, pt_regs, a0);
	OFFSET(PT_A1, pt_regs, a1);
	OFFSET(PT_A2, pt_regs, a2);
	OFFSET(PT_A3, pt_regs, a3);
	OFFSET(PT_A4, pt_regs, a4);
	OFFSET(PT_A5, pt_regs, a5);
	OFFSET(PT_A6, pt_regs, a6);
	OFFSET(PT_A7, pt_regs, a7);
	OFFSET(PT_T0, pt_regs, t0);
	OFFSET(PT_T1, pt_regs, t1);
	OFFSET(PT_T2, pt_regs, t2);
	OFFSET(PT_T3, pt_regs, t3);
	OFFSET(PT_T4, pt_regs, t4);
	OFFSET(PT_T5, pt_regs, t5);
	OFFSET(PT_T6, pt_regs, t6);
	OFFSET(PT_GP, pt_regs, gp);
	OFFSET(PT_ORIG_A0, pt_regs, orig_a0);
	OFFSET(PT_STATUS, pt_regs, status);
	OFFSET(PT_BADADDR, pt_regs, badaddr);
	OFFSET(PT_CAUSE, pt_regs, cause);

	// OFFSET(SUSPEND_CONTEXT_REGS, suspend_context, regs);

	// OFFSET(KVM_ARCH_GUEST_ZERO, kvm_vcpu_arch, guest_context.zero);
	// OFFSET(KVM_ARCH_GUEST_RA, kvm_vcpu_arch, guest_context.ra);
	// OFFSET(KVM_ARCH_GUEST_SP, kvm_vcpu_arch, guest_context.sp);
	// OFFSET(KVM_ARCH_GUEST_GP, kvm_vcpu_arch, guest_context.gp);
	// OFFSET(KVM_ARCH_GUEST_TP, kvm_vcpu_arch, guest_context.tp);
	// OFFSET(KVM_ARCH_GUEST_T0, kvm_vcpu_arch, guest_context.t0);
	// OFFSET(KVM_ARCH_GUEST_T1, kvm_vcpu_arch, guest_context.t1);
	// OFFSET(KVM_ARCH_GUEST_T2, kvm_vcpu_arch, guest_context.t2);
	// OFFSET(KVM_ARCH_GUEST_S0, kvm_vcpu_arch, guest_context.s0);
	// OFFSET(KVM_ARCH_GUEST_S1, kvm_vcpu_arch, guest_context.s1);
	// OFFSET(KVM_ARCH_GUEST_A0, kvm_vcpu_arch, guest_context.a0);
	// OFFSET(KVM_ARCH_GUEST_A1, kvm_vcpu_arch, guest_context.a1);
	// OFFSET(KVM_ARCH_GUEST_A2, kvm_vcpu_arch, guest_context.a2);
	// OFFSET(KVM_ARCH_GUEST_A3, kvm_vcpu_arch, guest_context.a3);
	// OFFSET(KVM_ARCH_GUEST_A4, kvm_vcpu_arch, guest_context.a4);
	// OFFSET(KVM_ARCH_GUEST_A5, kvm_vcpu_arch, guest_context.a5);
	// OFFSET(KVM_ARCH_GUEST_A6, kvm_vcpu_arch, guest_context.a6);
	// OFFSET(KVM_ARCH_GUEST_A7, kvm_vcpu_arch, guest_context.a7);
	// OFFSET(KVM_ARCH_GUEST_S2, kvm_vcpu_arch, guest_context.s2);
	// OFFSET(KVM_ARCH_GUEST_S3, kvm_vcpu_arch, guest_context.s3);
	// OFFSET(KVM_ARCH_GUEST_S4, kvm_vcpu_arch, guest_context.s4);
	// OFFSET(KVM_ARCH_GUEST_S5, kvm_vcpu_arch, guest_context.s5);
	// OFFSET(KVM_ARCH_GUEST_S6, kvm_vcpu_arch, guest_context.s6);
	// OFFSET(KVM_ARCH_GUEST_S7, kvm_vcpu_arch, guest_context.s7);
	// OFFSET(KVM_ARCH_GUEST_S8, kvm_vcpu_arch, guest_context.s8);
	// OFFSET(KVM_ARCH_GUEST_S9, kvm_vcpu_arch, guest_context.s9);
	// OFFSET(KVM_ARCH_GUEST_S10, kvm_vcpu_arch, guest_context.s10);
	// OFFSET(KVM_ARCH_GUEST_S11, kvm_vcpu_arch, guest_context.s11);
	// OFFSET(KVM_ARCH_GUEST_T3, kvm_vcpu_arch, guest_context.t3);
	// OFFSET(KVM_ARCH_GUEST_T4, kvm_vcpu_arch, guest_context.t4);
	// OFFSET(KVM_ARCH_GUEST_T5, kvm_vcpu_arch, guest_context.t5);
	// OFFSET(KVM_ARCH_GUEST_T6, kvm_vcpu_arch, guest_context.t6);
	// OFFSET(KVM_ARCH_GUEST_SEPC, kvm_vcpu_arch, guest_context.sepc);
	// OFFSET(KVM_ARCH_GUEST_SSTATUS, kvm_vcpu_arch, guest_context.sstatus);
	// OFFSET(KVM_ARCH_GUEST_HSTATUS, kvm_vcpu_arch, guest_context.hstatus);
	// OFFSET(KVM_ARCH_GUEST_SCOUNTEREN, kvm_vcpu_arch, guest_csr.scounteren);

	// OFFSET(KVM_ARCH_HOST_ZERO, kvm_vcpu_arch, host_context.zero);
	// OFFSET(KVM_ARCH_HOST_RA, kvm_vcpu_arch, host_context.ra);
	// OFFSET(KVM_ARCH_HOST_SP, kvm_vcpu_arch, host_context.sp);
	// OFFSET(KVM_ARCH_HOST_GP, kvm_vcpu_arch, host_context.gp);
	// OFFSET(KVM_ARCH_HOST_TP, kvm_vcpu_arch, host_context.tp);
	// OFFSET(KVM_ARCH_HOST_T0, kvm_vcpu_arch, host_context.t0);
	// OFFSET(KVM_ARCH_HOST_T1, kvm_vcpu_arch, host_context.t1);
	// OFFSET(KVM_ARCH_HOST_T2, kvm_vcpu_arch, host_context.t2);
	// OFFSET(KVM_ARCH_HOST_S0, kvm_vcpu_arch, host_context.s0);
	// OFFSET(KVM_ARCH_HOST_S1, kvm_vcpu_arch, host_context.s1);
	// OFFSET(KVM_ARCH_HOST_A0, kvm_vcpu_arch, host_context.a0);
	// OFFSET(KVM_ARCH_HOST_A1, kvm_vcpu_arch, host_context.a1);
	// OFFSET(KVM_ARCH_HOST_A2, kvm_vcpu_arch, host_context.a2);
	// OFFSET(KVM_ARCH_HOST_A3, kvm_vcpu_arch, host_context.a3);
	// OFFSET(KVM_ARCH_HOST_A4, kvm_vcpu_arch, host_context.a4);
	// OFFSET(KVM_ARCH_HOST_A5, kvm_vcpu_arch, host_context.a5);
	// OFFSET(KVM_ARCH_HOST_A6, kvm_vcpu_arch, host_context.a6);
	// OFFSET(KVM_ARCH_HOST_A7, kvm_vcpu_arch, host_context.a7);
	// OFFSET(KVM_ARCH_HOST_S2, kvm_vcpu_arch, host_context.s2);
	// OFFSET(KVM_ARCH_HOST_S3, kvm_vcpu_arch, host_context.s3);
	// OFFSET(KVM_ARCH_HOST_S4, kvm_vcpu_arch, host_context.s4);
	// OFFSET(KVM_ARCH_HOST_S5, kvm_vcpu_arch, host_context.s5);
	// OFFSET(KVM_ARCH_HOST_S6, kvm_vcpu_arch, host_context.s6);
	// OFFSET(KVM_ARCH_HOST_S7, kvm_vcpu_arch, host_context.s7);
	// OFFSET(KVM_ARCH_HOST_S8, kvm_vcpu_arch, host_context.s8);
	// OFFSET(KVM_ARCH_HOST_S9, kvm_vcpu_arch, host_context.s9);
	// OFFSET(KVM_ARCH_HOST_S10, kvm_vcpu_arch, host_context.s10);
	// OFFSET(KVM_ARCH_HOST_S11, kvm_vcpu_arch, host_context.s11);
	// OFFSET(KVM_ARCH_HOST_T3, kvm_vcpu_arch, host_context.t3);
	// OFFSET(KVM_ARCH_HOST_T4, kvm_vcpu_arch, host_context.t4);
	// OFFSET(KVM_ARCH_HOST_T5, kvm_vcpu_arch, host_context.t5);
	// OFFSET(KVM_ARCH_HOST_T6, kvm_vcpu_arch, host_context.t6);
	// OFFSET(KVM_ARCH_HOST_SEPC, kvm_vcpu_arch, host_context.sepc);
	// OFFSET(KVM_ARCH_HOST_SSTATUS, kvm_vcpu_arch, host_context.sstatus);
	// OFFSET(KVM_ARCH_HOST_HSTATUS, kvm_vcpu_arch, host_context.hstatus);
	// OFFSET(KVM_ARCH_HOST_SSCRATCH, kvm_vcpu_arch, host_sscratch);
	// OFFSET(KVM_ARCH_HOST_STVEC, kvm_vcpu_arch, host_stvec);
	// OFFSET(KVM_ARCH_HOST_SCOUNTEREN, kvm_vcpu_arch, host_scounteren);

	// OFFSET(KVM_ARCH_TRAP_SEPC, kvm_cpu_trap, sepc);
	// OFFSET(KVM_ARCH_TRAP_SCAUSE, kvm_cpu_trap, scause);
	// OFFSET(KVM_ARCH_TRAP_STVAL, kvm_cpu_trap, stval);
	// OFFSET(KVM_ARCH_TRAP_HTVAL, kvm_cpu_trap, htval);
	// OFFSET(KVM_ARCH_TRAP_HTINST, kvm_cpu_trap, htinst);



}