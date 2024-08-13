#include <syscall_init.h>
#include <xkernel/thread.h>
#include <xkernel/stdio.h>
#include <asm/stdio.h>
#include <fork.h>
#include <asm/timer.h>
#include <xkernel/console.h>
#include <fs/syscall_fs.h>
#include <xkernel/wait.h>
#include <xkernel/mmap.h>
#include <exec.h>
#include <xkernel/uname.h>
#include <fs/fd.h>

void sys_pstr(char *str)
{
	print_str(str);
}

void sys_person(void)
{
	printk("person\n");
}

void syscall_init(void)
{
	printk("syscall init start\n");
	syscall_table[SYS_PP]                 = sys_person;
	syscall_table[SYS_write]              = sys_write;
	syscall_table[SYS_sigreturn]          = sys_sigreturn;
	syscall_table[SYS_fcntl]              = sys_fcntl;
	syscall_table[SYS_sendfile]           = sys_sendfile;
	syscall_table[SYS_kill]               = sys_kill;
	syscall_table[SYS_mprotect]           = sys_mprotect;
	syscall_table[SYS_writev]             = sys_writev;
	syscall_table[SYS_ppoll]              = sys_ppoll;
	syscall_table[SYS_readlinkat]         = sys_readlinkat;
	syscall_table[SYS_sigaction]          = sys_sigaction;
	syscall_table[SYS_getpid]             = sys_getpid;
	syscall_table[SYS_getppid]            = sys_getppid;
	syscall_table[SYS_getgid]             = sys_getgid;
	syscall_table[SYS_gettimeofday]       = sys_gettimeofday;
	syscall_table[SYS_nanosleep]          = sys_sleep;
	syscall_table[SYS_exit]               = sys_exit;
	syscall_table[SYS_wait4]              = sys_wait;
	syscall_table[SYS_execve]             = sys_execve;
	syscall_table[SYS_getcwd]             = sys_getcwd;
	syscall_table[SYS_close]              = sys_close;
	syscall_table[SYS_read]               = sys_read;
	syscall_table[SYS_chdir]              = sys_chdir;
	syscall_table[SYS_dup]                = sys_dup;
	syscall_table[SYS_dup2]               = sys_dup2;
	syscall_table[SYS_fstat]              = sys_fstat;
	syscall_table[SYS_openat]             = sys_openat;
	syscall_table[SYS_mkdirat]            = sys_mkdirat;
	syscall_table[SYS_clone]              = sys_fork;
	syscall_table[SYS_unlinkat]           = sys_unlinkat;
	syscall_table[SYS_mount]              = sys_mount;
	syscall_table[SYS_umount2]            = sys_umount;
	syscall_table[SYS_sched_yield]        = thread_yield;
	syscall_table[SYS_PSTR]               = sys_pstr;
	syscall_table[SYS_mmap]               = sys_mmap;
	syscall_table[SYS_brk]                = sys_brk;
	syscall_table[SYS_munmap]             = sys_munmap;
	syscall_table[SYS_set_tid_address]    = sys_set_tid_address;
	syscall_table[SYS_times]              = sys_times;
	syscall_table[SYS_uname]              = sys_uname;
	syscall_table[SYS_statx]              = sys_statx;
	syscall_table[SYS_getdents64]         = sys_getdents;
	syscall_table[SYS_pipe2]              = sys_pipe;
	printk("syscall init done\n");
}


