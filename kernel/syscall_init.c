#include <syscall_init.h>
#include <linux/thread.h>
#include <linux/stdio.h>
#include <asm/stdio.h>
#include <fork.h>
#include <linux/console.h>

pid_t sys_getpid(void)
{
	struct task_struct* cur = running_thread();
	return cur->pid;
}

int64_t sys_write(int fd,const void* buf,size_t count)
{
	console_put_str((char*)buf);
	return count;
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
	syscall_table[SYS_PSTR]     = sys_pstr;
	syscall_table[SYS_FORK]     = sys_fork;

	printk("syscall init done\n");
}


