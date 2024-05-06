#include "printf.h"
#include <asm/syscall.h>

int main(void)
{
	int pid = fork();
	while(1){
		if (pid == 0){
			myprintf("this is child here\n");
		}
		else {
			myprintf("this is father here\n");
		}
	}
}
