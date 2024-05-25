#include <xkernel/mmap.h>
#include <xkernel/stdio.h>
#include <asm/syscall.h>
#include <fs/fd.h>
#include "xkernel/thread.h"

static struct kstat kst;
void test_mmap(void){
    running_thread()->pgdir = get_page();

    char *array;
    const char *str = "  Hello, mmap successfully!";
    int fd;

    fd = sys_open("test_mmap.txt", O_RDWR | O_CREATE, 0);
    sys_write(fd, str, strlen(str));
    sys_fstat(fd, &kst);
    printk("file len: %d\n", kst.st_size);
    array = (char *)sys_mmap(NULL, kst.st_size, PROT_WRITE | PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
    //printf("return array: %x\n", array);

    if (array == MAP_FAILED) {
	printk("mmap error.\n");
    }else{
	printk("mmap content: %s\n", array);
	//printf("%s\n", str);

	// sys_munmap(array, kst.st_size);
    }

    sys_close(fd);

}