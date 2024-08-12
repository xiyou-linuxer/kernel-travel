//#include <stdio.h>
#include "printf.h"
#include "xkernel/types.h"
#include <asm/syscall.h>

void h(int sig) {
	for (int i = 0; i < 5000; i++)
		myprintf("sigaction handler sig:%d\n",sig);
}


int main(void)
{
	int count = 0 ;
	char filepath[64];

	//printf("hello-you\n");
	//umemset(filepath,0,sizeof(filepath));
	//ustrcpy(filepath,"/sdcard/busybox");
	//char *argv[] = {"/sdcard/busybox","sh",NULL};
	//char *envp[] = {NULL};
	//ustrcpy(argv[1],"sh");
	
	struct k_sigaction act;
	act.sa_handler = h;
	myprintf("hello-you\n\n");
	umemset(filepath,0,sizeof(filepath));
	ustrcpy(filepath,"/sdcard/busybox");
	char *argv[] = {"/sdcard/busybox","sh","busybox_testcode.sh",NULL};
	char *envp[] = {NULL};
	ustrcpy(argv[1],"sh");
	ustrcpy(argv[2],"busybox_testcode.sh");
	//myprintf("hello-you\n\n");
	int pid = fork();
	if (pid == 0){
		sigaction(2,&act,NULL);
		while(1) {
			int i=0x100000;
			while(i--);
			myprintf("hello-everyone,I'm child,pid=%d\n",getpid());
		}
		//execve(filepath,(char**)argv,envp);
	}
	kill(pid,2);


	//int status;
	//int childpid = wait(&status);
	/*for (int i = 0; i < NR_SYSCALLS; i++)
	{
		
		//if (childpid == -1) {
		//	myprintf("no child now\n");
		//} else {
		//	myprintf("child %d status=%d\n",childpid,status);
		//}
		count++;
	}*/
	//write(1,"aaaaaaaaaaaaaaaaaaa\n",64);
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
