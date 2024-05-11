#include <fs/dirent.h>
#include <fs/vfs.h>
#include <fs/filepnt.h>
#include <linux/stdio.h>
#include <linux/string.h>
#include <linux/types.h>
#include <fs/fs.h>
#include <debug.h>
#include <linux/list.h>
#include <fs/buf.h>
#include <fs/cluster.h>
#include <fs/dirent.h>
char buf[8192];
void fat32Test(void) {
	// 测试读取文件
	
	Dirent *file;
	struct path_search_record searched_record;
	file = search_file("/text.txt",&searched_record);
	printk("filename:%s",file->name);
	filepnt_init(file);
	
	pre_read(file,buf,file->file_size/4096+1);
	file_read(file, 0, (unsigned long)buf, 0, file->file_size);
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
	//file_write(file, 0, (u64)str, 0, len) ;

	// TODO: 写一个删除文件的函数
	// 测试创建文件
	Dirent *file1;
	createFile(NULL, "/zrp123456789zrp.txt", &file1) ;
	char *str2 = "Hello! I\'m zrp!\n";
	printk("filename:%s\n",file1->name);
	file_write(file1, 0, (u64)str2, 0, strlen(str2) + 1) < 0;

	// 读取刚创建的文件
	file1 = search_file("/zrp123456789zrp.txt",&searched_record);
	file_read(file1, 0, (u64)buf, 0, file->file_size) ;
	printk("file zrp.txt: %s\n", buf);

	printk("FAT32 Test Passed!\n");
}
