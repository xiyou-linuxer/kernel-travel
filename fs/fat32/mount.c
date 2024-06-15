#include <fs/dirent.h>
#include <fs/fat32.h>
#include <fs/fs.h>
#include <fs/vfs.h>
#include <fs/fd.h>
#include <fs/path.h>
#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <sync.h>
#include <xkernel/errno.h>
#include <xkernel/thread.h>
extern struct lock mtx_file;

// mount之后，目录中原有的文件将被暂时取代为挂载的文件系统内的内容，umount时会重新出现
int mount_fs(char *special, char *dirPath) {
	// 1. 寻找mount的目录
	Dirent *dir;
	int fd = -1;
	struct path_search_record searched_record;
	memset(&searched_record, 0, sizeof(struct path_search_record));

	/* 记录目录深度.帮助判断中间某个目录不存在的情况 */
	unsigned int pathname_depth = path_depth_cnt((char *)dirPath);

	/* 先检查是否将全部的路径遍历 */
	dir = search_file(dirPath,&searched_record);
	unsigned int path_searched_depth = path_depth_cnt(searched_record.searched_path);
	if (pathname_depth != path_searched_depth)
	{ 
		// 说明并没有访问到全部的路径,某个中间目录是不存在的
		//printk("cannot access %s: Not a directory, subpath %s is`t exist\n",pathname, searched_record.searched_path);
		return -1;
	}
	// 检查dir是否是目录
	if (!is_directory(&dir->raw_dirent)) {
		printk("dir %s is not a directory!\n", dirPath);
		return -ENOTDIR;
	}

	// 2. 寻找mount的文件
	// 特判是否是设备（deprecated）
	Dirent *image;
	if (strncmp(special, "/dev/vda2", 10) == 0) {
		image = NULL;
	}

	// 3. 初始化mount的文件系统
	FileSystem *fs;
	allocFs(&fs);
	fs->image = image;
	fs->deviceNumber = 0;
	fs->mountPoint = dir;
	// 4. 将fs挂载到dir上
	dir->head = fs;
	dir->file_system = fs;

	return 0;
}

int umount_fs(char *dirPath) {
	// 1. 寻找mount的目录
	Dirent *dir;
	int fd = -1;
	struct path_search_record searched_record;
	memset(&searched_record, 0, sizeof(struct path_search_record));

	/* 记录目录深度.帮助判断中间某个目录不存在的情况 */
	unsigned int pathname_depth = path_depth_cnt((char *)dirPath);

	/* 先检查是否将全部的路径遍历 */
	dir = search_file(dirPath,&searched_record);
	unsigned int path_searched_depth = path_depth_cnt(searched_record.searched_path);
	if (pathname_depth != path_searched_depth)
	{ 
		return -1;
	}

	// 2. 擦除目录的标记
	// 要umount的目录一般使用getFile加载出来的是其文件系统的根目录，不能直接写回
	Dirent *mntPoint = dir->file_system->mountPoint;
	mntPoint->head = NULL;

	if (mntPoint == NULL || dir->parent_dirent != NULL) {
		//printk("unmounted dir!\n");
		return 0; // 传入的不是挂载点
	}
	
	// 3. 寻找fs
	FileSystem *fs = find_fs_by(find_fs_of_dir, mntPoint);
	if (fs == NULL) {
		//printk("can\'t find fs of dir %s!\n", dirPath);
		return 0;
	}
	deAllocFs(fs);

	return 0;
}