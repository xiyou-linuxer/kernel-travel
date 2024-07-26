#include "printf.h"
#include "xkernel/types.h"
#include <asm/syscall.h>


struct tms mytimes;
int main(void)
{
	//char filename[33][14];
	//umemset(filename,0,sizeof(filename));
	int count = 0 ;
	char filepath[30];

	umemset(filepath,0,sizeof(filepath));
	ustrcpy(filepath,"/sdcard/hello");
	char* argv[10];
	ustrcpy(argv[0],"/sdcard/hello");
	ustrcpy(argv[1],"sh");
	int pid = fork();
		if (pid == 0){
			execve(filepath,argv,NULL);
		}

		int status;
		int childpid = wait(&status);
	/*for (int i = 0; i < NR_SYSCALLS; i++)
	{
		
		//if (childpid == -1) {
		//	myprintf("no child now\n");
		//} else {
		//	myprintf("child %d status=%d\n",childpid,status);
		//}
		count++;
	}*/
	write(1,"aaaaaaaaaaaaaaaaaaa\n",64);
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
