//#include <stdio.h>
#include "printf.h"
#include "xkernel/types.h"
#include <asm/syscall.h>

#define AT_OPEN -10
#define SOURCEFILE "/sdcard/busybox_cmd.txt"
#define DESTFILE "/sdcard/busybox_testcode.sh"

int main(void)
{
	//int count = 0 ;
	//char filepath[64];
	char buf[64];
	//umemset(filepath,0,sizeof(filepath));
	//ustrcpy(filepath,"/sdcard/busybox");
	//char *argv[] = {"/sdcard/busybox","sh","busybox_testcode.sh",NULL};
	//char *envp[] = {NULL};

	////myprintf("hello-you\n\n");
	//int pid = fork();
	//if (pid == 0){
	//	execve(filepath,(char**)argv,envp);
	//}


	int p[2];
	off_t in_off,out_off;
	pipe(p);
	int fd = openfile(AT_OPEN,SOURCEFILE,3,0);
	pstr("\n================test1==============\n");
	splice(fd,&in_off,p[0],NULL,10,0);
	if (in_off == 10) {
		pstr("offset good");
	}
	readfile(p[1],buf,10);
	buf[10]=0;
	pstr("first splice:\n");
	pstr(buf);
	pstr("\n");

	splice(fd,&in_off,p[0],NULL,10,0);
	readfile(p[1],buf,10);
	buf[10]=0;
	pstr("second splice:\n");
	pstr(buf);
	pstr("\n");

	pstr("================test2==============\n");
	long written;
	splice(fd,&in_off,p[0],NULL,10,0);
	int outfd = openfile(AT_OPEN,DESTFILE,3,0);
	splice(p[1],NULL,outfd,&written,10,0);
	readfile(outfd,buf,10);
	buf[5]=0;
	if (!ustrcmp("01234",buf)) {
		pstr("pass\n");
	}

	pstr("================test3==============\n");
	pstr("file bytes remain smaller than len\n");
	splice(fd,&in_off,p[0],NULL,100,0);
	readfile(p[1],buf,30);
	buf[30]=0;
	pstr(buf);
	pstr("\n");

	pstr("================test4==============\n");
	pstr("pipe bytes remain smaller than len");
	splice(fd,&in_off,p[0],NULL,10,0);
	splice(p[1],NULL,outfd,&out_off,100,0);
	readfile(outfd,buf,10);
	buf[10]=0;
	pstr(buf);

	//splice()

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
