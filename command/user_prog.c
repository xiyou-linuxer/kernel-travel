#include "printf.h"
#include <asm/syscall.h>

char* sysname[NR_SYSCALLS] = {
	//[SYS_getpid]       = "getpid",
	//[SYS_gettimeofday] = "gettimeofday",
	//[SYS_nanosleep]    = "sleep",
	[SYS_wait4]        = "wait",
	//[SYS_clone]        = "fork",
	//[SYS_write]        = "write",
	//[SYS_getcwd]       = "getcwd",
	//[SYS_chdir]        = "chdir",
	//[SYS_dup2]         = "dup2",
	//[SYS_dup]          = "dup",
	//[SYS_fstat]        = "fstat",
	//[SYS_close]        = "close",
	//[SYS_openat]       = "open",
	//[SYS_read]         = "read",
	//[SYS_mkdirat]      = "mkdir_"
	//[SYS_unlinkat]     = "unlink",
//	[SYS_mount]        = "mount",
//	[SYS_umount2]      = "umount",
//	[1]                = "openat",
};



int main(void)
{
	struct timespec req;

	char filename[50][64];
	umemset(filename,0,sizeof(filename));
	int count = 0 ;
	for (int i = 0; i < NR_SYSCALLS; i++)
	{
		if (sysname[i] == NULL)
			continue;
		ustrcpy(filename[count],"/");
		ustrcat(filename[count],sysname[i]);

		int pid = fork();
		if (pid == 0){
			execve(filename[count],NULL,NULL);
		}
		int status;
		int childpid = wait(&status);
		myprintf("this is father here\n");
		if (childpid == -1) {
			myprintf("no child now\n");
		} else {
			myprintf("child %d status=%d\n",childpid,status);
		}
		count++;
	}
}
