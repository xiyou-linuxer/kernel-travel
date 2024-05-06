#include <fs/dirent.h>
#include <fs/vfs.h>
#include <linux/stdio.h>
#include <linux/string.h>
#include <linux/types.h>
#include <fs/fs.h>
#include <debug.h>
char buf[8192];
void fat32Test(void) {
	// 测试读取文件
	Dirent *file;
	ASSERT(getFile(NULL, "/text.txt", &file)==0);
	(file_read(file, 0, (u64)buf, 0, file->file_size) < 0);
	printk("%s\n", buf);

	// 测试写入文件
	char *str = "Hello! I\'m "
		    "zrp!"
		    "\n3233333333233333333233333333233333333233333333233333333233333333233333333233"
		    "333333233333333233333333233333333233333333233333333233333333233333333233333333"
		    "233333333233333333222222222233233333333233333333233333333233333333233333333233"
		    "333333233333333233333333233333333233333333233333333233333333233333333233333333"
		    "233333333233333333222222222233233333333233333333233333333233333333233333333233"
		    "333333233333333233333333233333333233333333233333333233333333233333333233333333"
		    "233333333233333333222222222233233333333233333333233333333233333333233333333233"
		    "333333233333333233333333233333333233333333233333333233333333233333333233333333"
		    "23333333323333333322222222222222222222222222\n This is end!";
	int len = strlen(str) + 1;
	ASSERT(file_write(file, 0, (u64)str, 0, len) < 0);

	// 读出文件
	ASSERT(file_read(file, 0, (u64)buf, 0, file->file_size) < 0);
	printk("%s\n", buf);

	// TODO: 写一个删除文件的函数
	// 测试创建文件
	ASSERT(createFile(NULL, "/zrp123456789zrp.txt", &file) < 0);
	char *str2 = "Hello! I\'m zrp!\n";
	ASSERT(file_write(file, 0, (u64)str2, 0, strlen(str2) + 1) < 0);

	// 读取刚创建的文件
	ASSERT(getFile(NULL, "/zrp123456789zrp.txt", &file)==0);
	ASSERT(file_read(file, 0, (u64)buf, 0, file->file_size) < 0);
	printk("file zrp.txt: %s\n", buf);

	printk("FAT32 Test Passed!\n");
}
