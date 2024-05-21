#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <xkernel/types.h>
#include <xkernel/thread.h>
#include <xkernel/list.h>
#include <xkernel/console.h>
#include <xkernel/string.h>
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

/*void test_open(void) {
    // O_RDONLY = 0, O_WRONLY = 1
    int fd = sys_open("./text.txt", O_RDWR ,660);
    ASSERT(fd >= 0);
    
    char buf[256];
    int size = sys_read(fd, buf, 256);
	printk("sys_open\n");
    if (size < 0) {
        size = 0;
    }
    sys_write(STDOUT, buf, size);
    sys_close(fd);
	printk("sys_close\n");
}

void test_close(void) {
	const char *str = "  close error.\n";
	int str_len = strlen(str);
    printk("str_len:%d\n", str_len);
    int fd = sys_open("test_close1.txt", O_CREATE | O_RDWR,660);
    ASSERT(fd > 0);
    ASSERT(sys_write(fd, str, str_len) == str_len);
    sys_write(fd, str, str_len);
    int rt = sys_close(fd);
    printk("rt :%d", rt);
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

void test_mkdir(void){
    int rt, fd;
    rt = sys_mkdir("test_mkdir1", 0666);
    printk("mkdir ret: %d\n", rt);
    ASSERT(rt != -1);
    fd = sys_open("test_mkdir1", O_RDONLY | O_DIRECTORY,660);
    if(fd > 0){
        printk("  mkdir success.\n");
        sys_close(fd);
    }
    else printk("  mkdir error.\n");
}

void test_chdir(void){
    char buffer[30];
    sys_mkdir("/test_chdir", 0666);
    int ret = sys_chdir("/test_chdir");
    printk("chdir ret: %d\n", ret);
    ASSERT(ret == 0);
    sys_getcwd(buffer, 30);
    printk("  current working dir : %s\n", buffer);
}

void test_fstat(void) 
{
	int fd = sys_open("./text.txt", 0,660);
	struct kstat kst;
	int ret = sys_fstat(fd, &kst);
	printk("fstat ret: %d\n", ret);
	ASSERT(ret >= 0);

	printk("fstat: dev: %d, inode: %d, mode: %d, nlink: %d, size: %d, atime: %d, mtime: %d, ctime: %d\n",
	      kst.st_dev, kst.st_ino, kst.st_mode, kst.st_nlink, kst.st_size, kst.st_atime_sec, kst.st_mtime_sec, kst.st_ctime_sec);
}

void test_lseek(void)
{
	int fd = sys_open("text.txt", O_RDWR,660);
	ASSERT(fd > 0);
	sys_write(fd, "Catch me if you can!\n", 21);
	sys_lseek(fd, 0, SEEK_CUR);
	char buf[32] = {0};
	sys_read(fd, buf, 21); 
	printk("%s\n",buf);
}

void test_dup(void){
	int fd = sys_dup(STDOUT);
	ASSERT(fd >=0);
	printk("  new fd is %d.\n", fd);
}

void test_dup2(void){
	int fd = sys_dup2(STDOUT, 100);
	ASSERT(fd != -1);
	const char *str = "  from fd 100\n";
	sys_write(100, str, strlen(str));
}*/
void test_unlink(void)
{

    char *fname = "./test_unlink";
    int fd, ret;

    fd = sys_open(fname, O_CREATE | O_WRONLY,660);
    ASSERT(fd > 0);
    sys_close(fd);

    // unlink test
    ret = sys_unlink(fname);
    ASSERT(ret == 0);
    fd = sys_open(fname, O_RDONLY,660);
    if(fd < 0){
        printk("  unlink success!\n");
    }else{
	printk("  unlink error!\n");
        sys_close(fd);
    }
    // It's Ok if you don't delete the inode and data blocks.
}
static char mntpoint[64] = "./mnt";
    static char device[64] = "/dev/vda2";
    static const char *fs_type = "vfat";
void test_mount(void) {


    
	printk("Mounting dev:%s to %s\n", device, mntpoint);
	int ret = sys_mount(device, mntpoint, fs_type, 0, NULL);
    printk("mount return: %d\n", ret);
	ASSERT(ret == 0);

	if (ret == 0) {
		printk("mount successfully\n");
		ret = sys_umount(mntpoint);
		printk("umount return: %d\n", ret);
	}
    
	
}
void test_openat(void) {
    int fd_dir = sys_open("./mnt", O_DIRECTORY,0);
    printk("open dir fd: %d\n", fd_dir);
    int fd = sys_openat(fd_dir, "test_openat.txt", O_CREATE | O_RDWR,0);
    printk("openat fd: %d\n", fd);
    ASSERT(fd > 0);
    printk("openat success.\n");
    sys_close(fd);	
}
void test_fs_all(void)
{
    test_mount();
    test_unlink();
    test_openat();
    while (1) {
    };
} 