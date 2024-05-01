#include <asm/syscall.h>
#include <asm/pt_regs.h>
#include <linux/types.h>

asmlinkage long sys_ni_syscall(void) {
	return -ENOSYS;
}

typedef long (*syscall_fn) (unsigned long,unsigned long,unsigned long,
							unsigned long,unsigned long,unsigned long);
//(unsigned long) X 6 => a7 + a0~a4

void* syscall_table[NR_SYSCALLS] = {
	[0 ... NR_SYSCALLS-1] = sys_ni_syscall
};


void __attribute__((__noinline__)) do_syscall(struct pt_regs *regs)
{
	unsigned long nr;
	syscall_fn sys_fn;

	nr = regs->regs[11];
	regs->csr_era += 4;  //加上syscall 0 的大小

	regs->regs[4] = -ENOSYS;
	if (nr > NR_SYSCALLS) {
		return ;
	}
	sys_fn = syscall_table[nr];
	regs->regs[4] = sys_fn(regs->regs[4],regs->regs[5],regs->regs[6], \
						   regs->regs[7],regs->regs[8],regs->regs[9]);
}

inline pid_t getpid(void) {
	return syscall(SYS_GETPID);
}

inline void pstr(char *str) {
	syscall(SYS_PSTR,str);
}

