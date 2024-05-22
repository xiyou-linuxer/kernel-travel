#include <asm/syscall.h>
#include <asm/pt_regs.h>
#include <xkernel/types.h>
#include <xkernel/stdio.h>

asmlinkage long sys_ni_syscall(void) {
	return -ENOSYS;
}

typedef long (*syscall_fn) (unsigned long,unsigned long,unsigned long,
							unsigned long,unsigned long,unsigned long);
//(unsigned long) X 6 => a7 + a0~a4

void* syscall_table[NR_SYSCALLS] = {
	[0 ... NR_SYSCALLS-1] = sys_ni_syscall
};

//char* sysname[NR_SYSCALLS] = {
//	[SYS_getpid]       = "getpid",
//	[SYS_gettimeofday] = "gettimeofday",
//	[SYS_nanosleep]    = "sleep",
//	[SYS_write]        = "write",
//	[SYS_getcwd]       = "getcwd",
//	[SYS_chdir]        = "chdir",
//	[SYS_dup2]         = "dup2",
//	[SYS_dup]          = "dup",
//	[SYS_fstat]        = "fstat",
//	[SYS_close]        = "close",
//	[SYS_openat]       = "open",
//	[SYS_read]         = "read",
//	[SYS_mkdirat]      = "mkdir_",
//	[SYS_unlinkat]     = "unlink",
//	[SYS_mount]        = "mount",
//	[SYS_umount2]      = "umount",
//	[1]                = "openat",
//};

void __attribute__((__noinline__)) do_syscall(struct pt_regs *regs)
{
	unsigned long nr;
	syscall_fn sys_fn;

	nr = regs->regs[11];
	regs->csr_era += 4;  //加上syscall 0 的大小

	regs->orig_a0 = regs->regs[4];
	regs->regs[4] = -ENOSYS;
	if (nr > NR_SYSCALLS) {
		return ;
	}
	sys_fn = syscall_table[nr];
	regs->regs[4] = sys_fn(regs->orig_a0,regs->regs[5],regs->regs[6], \
						   regs->regs[7],regs->regs[8],regs->regs[9]);
}

