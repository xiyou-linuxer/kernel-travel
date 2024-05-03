#include <syscall_init.h>
#include <linux/thread.h>
#include <linux/stdio.h>
#include <asm/stdio.h>

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
	syscall_table[SYS_GETPID]   = sys_getpid;
	syscall_table[SYS_PSTR]     = sys_pstr;

	printk("syscall init done\n");
}


