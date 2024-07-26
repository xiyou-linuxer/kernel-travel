#include "printf.h"
#include "xkernel/types.h"
#include <asm/syscall.h>


struct tms mytimes;
int main(void)
{
	void * map1 = mmap(NULL, 4096, 0, 50, -1, 0);
	void * map2 = mmap(NULL, 4095, 0, 50, -1, 0);
	void * map3 = mmap(NULL, 4094, 0, 50, -1, 0);

	myprintf("map : %p", map1);

}
