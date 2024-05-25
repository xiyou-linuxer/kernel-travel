#include "printf.h"
#include <asm/syscall.h>


char* sysname[NR_SYSCALLS] = {
	[SYS_getpid]       = "getpid",
	[SYS_getppid]      = "getppid",
	[SYS_gettimeofday] = "gettimeofday",
	[SYS_nanosleep]    = "sleep",
	[SYS_wait4]        = "wait",
	[SYS_clone]        = "fork",
	[SYS_write]        = "write",
	[SYS_getcwd]       = "getcwd",
	[SYS_chdir]        = "chdir",
	[SYS_dup2]         = "dup2",
	[SYS_dup]          = "dup",
	[SYS_fstat]        = "fstat",
	[SYS_close]        = "close",
	[SYS_openat]       = "open",
	[SYS_read]         = "read",
	[SYS_mkdirat]      = "mkdir_",
	[SYS_mount]        = "mount",
	[SYS_umount2]      = "umount",
	[SYS_sched_yield]  = "yield",
	[1]                = "openat",
	[2]                = "waitpid",
	[3]                = "clone",
	[SYS_unlinkat]     = "unlink",
	[SYS_exit]         = "exit",
	[SYS_times]        = "times",
	[SYS_execve]       = "execve",
	[SYS_uname]        = "uname",
	[SYS_mmap]         = "mmap",
	[SYS_brk]          = "brk",
	[SYS_munmap]       = "munmap",
};

struct tms mytimes;
int main(void)
{
	char filename[30][14];
	umemset(filename,0,sizeof(filename));
	int count = 0 ;

	for (int i = 0; i < NR_SYSCALLS; i++)
	{
		if (sysname[i] == NULL)
			continue;
		ustrcpy(filename[count],"/");
		ustrcat(filename[count],sysname[i]);

		//myprintf("next:%s \n",filename[count]);
		int pid = fork();
		if (pid == 0){
			execve(filename[count],NULL,NULL);
		}

		int status;
		int childpid = wait(&status);
		//if (childpid == -1) {
		//	myprintf("no child now\n");
		//} else {
		//	myprintf("child %d status=%d\n",childpid,status);
		//}
		count++;
	}
	while(1) {
		//int status;
		//int childpid = wait(&status);
		//if (childpid == -1) {
		//	//myprintf("no child now\n");
		//} else {
		//	//myprintf("child %d status=%d\n",childpid,status);
		//}
	}
}
