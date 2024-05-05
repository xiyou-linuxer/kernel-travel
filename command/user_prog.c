#include <asm/syscall.h>

int main(void)
{
	char* str = "hello world\n";
	while(1){
		pstr(str);
	}
}
