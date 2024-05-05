#include <fs/dirent.h>
#include <fs/fat32.h>
#include <fs/filedev.h>
#include <fs/fs.h>
#include <fs/vfs.h>
#include <linux/stdio.h>
#include <linux/string.h>
#include <sync.h>
#include <sys/errno.h>

extern struct lock mtx_file;

int is_mount_dir(Dirent *dirent)
{
	if(dirent->head != NULL){
		return true;
	}else
	{
		return false;
	}
	
}

// mount之后，目录中原有的文件将被暂时取代为挂载的文件系统内的内容，umount时会重新出现
int mount_fs(char *special, Dirent *baseDir, char *dirPath) {
	lock_acquire(&mtx_file);

	// 1. 寻找mount的目录
	Dirent *dir;
	int ret = getFile(baseDir, dirPath, &dir);

	if (ret < 0) {
		printk("dir %s is not found!\n", dirPath);
		lock_release(&mtx_file);
		return ret;
	}

	// 检查dir是否是目录
	if (!is_directory(&(dir->raw_dirent))) {
		printk("dir %s is not a directory!\n", dirPath);
		file_close(dir);
		lock_release(&mtx_file);
		return -ENOTDIR;
	}

	// 2. 寻找mount的文件
	// 特判是否是设备（deprecated）
	Dirent *image;
	if (strcmp(special, "/dev/vda2") == 0) {
		image = NULL;
	} else {
		ret = getFile(baseDir, special, &image);
		if (ret < 0) {
			warn("image %s is not found!\n", special);
			file_close(dir);
			mtx_unlock_sleep(&mtx_file);
			return ret;
		}
	}

	// 3. 初始化mount的文件系统
	FileSystem *fs;
	allocFs(&fs);
	fs->image = image;
	fs->deviceNumber = 0;
	fs->mountPoint = dir;
	fat32_init(fs);

	// 4. 将fs挂载到dir上
	dir->head = fs;

	lock_release(&mtx_file);
	return 0;
}

int umount_fs(char *dirPath, Dirent *baseDir) {
	lock_acquire(&mtx_file);

	// 1. 寻找mount的目录
	Dirent *dir;
	int ret = getFile(baseDir, dirPath, &dir);
	if (ret < 0) {
		printk("dir %s is not found!\n", dirPath);
		lock_release(&mtx_file);
		return ret;
	}

	// 2. 擦除目录的标记
	// 要umount的目录一般使用getFile加载出来的是其文件系统的根目录，不能直接写回
	Dirent *mntPoint = dir->file_system->mountPoint;
	if (mntPoint == NULL || dir->parent_dirent != NULL) {
		printk("unmounted dir!\n");
		lock_release(&mtx_file);
		return -EINVAL; // 传入的不是挂载点
	}
	mntPoint->head = NULL;

	// 3. 寻找fs
	FileSystem *fs = find_fs_by(find_fs_of_dir, mntPoint);
	if (fs == NULL) {
		printk("can\'t find fs of dir %s!\n", dirPath);
		lock_release(&mtx_file);
		return -EINVAL;
	}

	// 4. 关闭fs镜像（如果有）和挂载点，并卸载fs
	if (fs->image != NULL) {
		file_close(fs->image);
		file_close(dir);
	}
	deAllocFs(fs);

	lock_release(&mtx_file);
	return 0;
}
