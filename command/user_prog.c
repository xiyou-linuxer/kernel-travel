#include "printf.h"
#include <asm/syscall.h>

int main(void)
{
	int pid = fork();
	if (pid == 7){
		myprintf("this is child here\n");
	}
	else {
		myprintf("this is father here\n");
	}
}
