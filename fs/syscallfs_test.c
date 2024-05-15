#include <linux/stdio.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/thread.h>
#include <linux/list.h>
#include <linux/console.h>
#include <linux/string.h>
#include <fs/buf.h>
#include <fs/cluster.h>
#include <fs/dirent.h>
#include <fs/dirent.h>
#include <fs/vfs.h>
#include <fs/filepnt.h>
#include <fs/fs.h>
#include <fs/fd.h>
#include <fs/syscall_fs.h>
#include <debug.h>

void test_open(void) {
    // O_RDONLY = 0, O_WRONLY = 1
    int fd = sys_open("./text.txt", O_RDWR ,660);
    ASSERT(fd >= 0);
    printk("sys_open\n");
    char buf[256];
    int size = sys_read(fd, buf, 256);
    if (size < 0) {
        size = 0;
    }
    printk("%s\n", buf);
    sys_close(fd);
	printk("sys_close\n");
}

void test_close(void) {
    int fd = sys_open("test_close1.txt", O_CREATE | O_RDWR,660);
    ASSERT(fd > 0);
    const char *str = "  close error.\n";
    int str_len = strlen(str);
    printk("str_len:%d\n", 15);
    ASSERT(sys_write(fd, str, 15) == 15);
    sys_write(fd, str, 15);
    int rt = sys_close(fd);
    ASSERT(rt == 0);
    printk("  close %d success.\n", fd);
}

void test_getcwd(void){
    char *cwd = NULL;
    char buf[128] = {0};
    cwd = sys_getcwd(buf, 128);
    if(cwd != NULL) printk("getcwd: %s successfully!\n", buf);
    else printk("getcwd ERROR.\n");
}

void test_fs_all(void)
{
	test_getcwd();
	test_open();
    test_close();
}