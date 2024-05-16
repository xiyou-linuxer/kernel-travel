#include <syscall_init.h>
#include <xkernel/thread.h>
#include <xkernel/stdio.h>
#include <asm/stdio.h>
#include <fork.h>
#include <xkernel/console.h>
#include <fs/syscall_fs.h>

pid_t sys_getpid(void)
{
	struct task_struct* cur = running_thread();
	return cur->pid;
}

void sys_pstr(char *str)
{
	print_str(str);
}

void syscall_init(void)
{
	printk("syscall init start\n");
	syscall_table[SYS_write]    = sys_write;
    syscall_table[SYS_getpid]   = sys_getpid;
    syscall_table[SYS_getcwd]   = sys_getcwd;
    syscall_table[SYS_close]    = sys_close;
    syscall_table[SYS_read]     = sys_read;
	syscall_table[SYS_chdir]    = sys_chdir;
    syscall_table[SYS_PSTR]     = sys_pstr;
    syscall_table[SYS_FORK]     = sys_fork;

	printk("syscall init done\n");
}


