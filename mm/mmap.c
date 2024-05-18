#include <xkernel/mmap.h>
#include <xkernel/types.h>
#include <xkernel/memory.h>


void* sys_mmap(void* addr, size_t len, int prot,
		int flags, int fd, off_t pgoff)
{
	char * tmp = (char*)get_page();
	tmp = "mmap content:   Hello, mmap successfully!";
	return tmp;
}

int sys_munmap(void *start, size_t len)
{
	return 0;
}
